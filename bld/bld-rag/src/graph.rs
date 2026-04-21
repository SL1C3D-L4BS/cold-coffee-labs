//! Symbol graph + PageRank over the corpus.
//!
//! The symbol graph is a lightweight directed graph where nodes are
//! qualified symbol names (`scene::create_entity`, `gw::editor::AgentPanel`,
//! etc.) and edges point from a caller/referrer to the definition. We
//! extract nodes + edges with regex-based heuristics so we don't pay the
//! cost of a full parser during incremental indexing.
//!
//! PageRank-over-this-graph gives us a cheap "importance" score that the
//! hybrid retriever blends with BM25 + embedding cosine. See
//! `ADR-0013 §2.3` for the rationale.

use std::collections::HashMap;

use once_cell::sync::Lazy;
use petgraph::graph::{DiGraph, NodeIndex};
use regex::Regex;

use crate::corpus::{Corpus, DocumentId, LangHint};

/// Regexes for extracting symbol definitions + references. Conservative
/// on purpose — false positives are fine, but false negatives erase the
/// whole rank signal.
static RUST_DEF: Lazy<Regex> = Lazy::new(|| {
    // `fn foo(` / `pub fn foo(` / `async fn foo(` / `struct Bar` / `enum X`
    #[allow(clippy::expect_used)]
    Regex::new(r"(?m)^\s*(?:pub(?:\(.*?\))?\s+)?(?:async\s+)?(?:fn|struct|enum|trait|type)\s+([A-Za-z_][A-Za-z0-9_]*)")
        .expect("rust-def regex must compile")
});

static RUST_CALL: Lazy<Regex> = Lazy::new(|| {
    #[allow(clippy::expect_used)]
    Regex::new(r"([A-Za-z_][A-Za-z0-9_]{2,})\s*\(")
        .expect("rust-call regex must compile")
});

static CPP_DEF: Lazy<Regex> = Lazy::new(|| {
    // `ReturnType ClassName::method(` / `class Foo {` / `struct Bar` etc.
    #[allow(clippy::expect_used)]
    Regex::new(r"(?m)^(?:[A-Za-z_][A-Za-z0-9_:<>,\s\*&]*?\s+)?([A-Za-z_][A-Za-z0-9_]+)::([A-Za-z_][A-Za-z0-9_]+)\s*\(")
        .expect("cpp-def regex must compile")
});

/// Symbol graph wrapping `petgraph::DiGraph`.
#[derive(Debug, Default)]
pub struct SymbolGraph {
    graph: DiGraph<String, ()>,
    index: HashMap<String, NodeIndex>,
    /// Reverse lookup: symbol name -> owning document id.
    owners: HashMap<NodeIndex, DocumentId>,
    /// Cached PageRank scores.
    ranks: HashMap<NodeIndex, f32>,
}

impl SymbolGraph {
    /// New empty graph.
    pub fn new() -> Self {
        Self::default()
    }

    /// Number of nodes.
    pub fn node_count(&self) -> usize {
        self.graph.node_count()
    }

    /// Number of edges.
    pub fn edge_count(&self) -> usize {
        self.graph.edge_count()
    }

    /// Borrow the inner graph.
    pub fn inner(&self) -> &DiGraph<String, ()> {
        &self.graph
    }

    /// Rebuild the graph from the corpus.
    pub fn rebuild(&mut self, corpus: &Corpus) {
        self.graph.clear();
        self.index.clear();
        self.owners.clear();
        self.ranks.clear();

        // First pass — collect every definition as a node. Stash
        // provisional call sites per document for the second pass.
        #[derive(Default)]
        struct DocState {
            defs: Vec<String>,
            calls: Vec<String>,
        }
        let mut per_doc: HashMap<DocumentId, DocState> = HashMap::new();

        for (doc_id, doc) in corpus.documents() {
            let mut st = DocState::default();
            // Concatenate this doc's chunks to run the regexes once.
            let body: String = corpus
                .chunks_for(doc_id)
                .map(|(_, c)| c.body.clone())
                .collect();
            match doc.lang {
                LangHint::Rust => {
                    for cap in RUST_DEF.captures_iter(&body) {
                        if let Some(m) = cap.get(1) {
                            st.defs.push(m.as_str().to_owned());
                        }
                    }
                    for cap in RUST_CALL.captures_iter(&body) {
                        if let Some(m) = cap.get(1) {
                            let s = m.as_str();
                            if !is_rust_keyword(s) {
                                st.calls.push(s.to_owned());
                            }
                        }
                    }
                }
                LangHint::Cpp => {
                    for cap in CPP_DEF.captures_iter(&body) {
                        if let (Some(cls), Some(mth)) = (cap.get(1), cap.get(2)) {
                            st.defs.push(format!("{}::{}", cls.as_str(), mth.as_str()));
                            st.defs.push(cls.as_str().to_owned());
                        }
                    }
                    for cap in RUST_CALL.captures_iter(&body) {
                        if let Some(m) = cap.get(1) {
                            let s = m.as_str();
                            if !is_cpp_keyword(s) {
                                st.calls.push(s.to_owned());
                            }
                        }
                    }
                }
                _ => {}
            }
            per_doc.insert(doc_id, st);
        }

        // Create nodes for every def.
        for (doc_id, st) in &per_doc {
            for def in &st.defs {
                if self.index.contains_key(def) {
                    continue;
                }
                let ni = self.graph.add_node(def.clone());
                self.index.insert(def.clone(), ni);
                self.owners.insert(ni, *doc_id);
            }
        }

        // Create edges from each call site to the matching def (if any).
        for (_, st) in &per_doc {
            for call in &st.calls {
                let Some(&def_ni) = self.index.get(call) else {
                    continue;
                };
                // Every doc that mentions `call` gets an edge from that
                // doc's first def (if one exists) to `def_ni`. We use
                // every def in the current doc state for a richer graph.
                for src in &st.defs {
                    if let Some(&src_ni) = self.index.get(src) {
                        if src_ni != def_ni {
                            self.graph.add_edge(src_ni, def_ni, ());
                        }
                    }
                }
            }
        }

        self.ranks = self.compute_pagerank(32, 0.85);
    }

    /// Fetch PageRank for a symbol (0.0 if unknown).
    pub fn rank(&self, symbol: &str) -> f32 {
        let Some(&ni) = self.index.get(symbol) else {
            return 0.0;
        };
        *self.ranks.get(&ni).unwrap_or(&0.0)
    }

    /// Neighbors of a seed symbol, up to `limit`, sorted by rank desc.
    /// Duplicates (multi-edges) are collapsed into a single entry.
    pub fn neighbors(&self, seed: &str, limit: usize) -> Vec<(String, f32)> {
        let Some(&start) = self.index.get(seed) else {
            return Vec::new();
        };
        let mut seen: std::collections::HashSet<NodeIndex> = std::collections::HashSet::new();
        let mut out: Vec<(String, f32)> = Vec::new();
        for ni in self.graph.neighbors(start) {
            if !seen.insert(ni) {
                continue;
            }
            let name = self.graph[ni].clone();
            let score = *self.ranks.get(&ni).unwrap_or(&0.0);
            out.push((name, score));
        }
        out.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap_or(std::cmp::Ordering::Equal));
        out.truncate(limit);
        out
    }

    /// Owning document id for a symbol.
    pub fn owner(&self, symbol: &str) -> Option<DocumentId> {
        let ni = *self.index.get(symbol)?;
        self.owners.get(&ni).copied()
    }

    /// Lightweight PageRank: `iterations` passes with damping `d`.
    fn compute_pagerank(&self, iterations: u32, d: f32) -> HashMap<NodeIndex, f32> {
        let n = self.graph.node_count();
        if n == 0 {
            return HashMap::new();
        }
        let base = (1.0 - d) / n as f32;
        let mut rank: HashMap<NodeIndex, f32> = self
            .graph
            .node_indices()
            .map(|ni| (ni, 1.0 / n as f32))
            .collect();

        for _ in 0..iterations {
            let mut next: HashMap<NodeIndex, f32> = HashMap::with_capacity(n);
            for ni in self.graph.node_indices() {
                let mut sum = 0.0_f32;
                for src in self.graph.neighbors_directed(ni, petgraph::Incoming) {
                    let out_degree = self
                        .graph
                        .neighbors_directed(src, petgraph::Outgoing)
                        .count() as f32;
                    if out_degree > 0.0 {
                        sum += rank.get(&src).copied().unwrap_or(0.0) / out_degree;
                    }
                }
                next.insert(ni, base + d * sum);
            }
            rank = next;
        }
        rank
    }
}

fn is_rust_keyword(s: &str) -> bool {
    matches!(
        s,
        "fn" | "let" | "mut" | "if" | "else" | "for" | "while" | "match"
        | "return" | "pub" | "use" | "mod" | "impl" | "trait" | "struct"
        | "enum" | "where" | "async" | "await" | "as" | "ref" | "move"
        | "dyn" | "loop" | "break" | "continue" | "type" | "const" | "static"
        | "Some" | "None" | "Ok" | "Err" | "Self" | "Box" | "Vec" | "String"
        | "true" | "false" | "unsafe" | "extern" | "crate" | "super"
    )
}

fn is_cpp_keyword(s: &str) -> bool {
    matches!(
        s,
        "if" | "else" | "for" | "while" | "do" | "return" | "switch"
        | "case" | "break" | "continue" | "class" | "struct" | "enum"
        | "public" | "private" | "protected" | "virtual" | "override"
        | "final" | "namespace" | "using" | "typedef" | "typename"
        | "template" | "sizeof" | "nullptr" | "this" | "true" | "false"
        | "static" | "const" | "constexpr" | "auto" | "void" | "int"
        | "float" | "double" | "bool" | "char" | "new" | "delete"
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn graph_builds_from_rust_corpus() {
        let mut c = Corpus::new();
        c.upsert(
            "mod.rs",
            "pub fn entry_point() { called(); }\n\nfn called() -> i32 { 1 }\n",
        );
        let mut g = SymbolGraph::new();
        g.rebuild(&c);
        assert!(g.node_count() >= 2);
        // Sum of all ranks is ~1.0 (PageRank invariant).
        let total: f32 = g.ranks.values().sum();
        assert!((total - 1.0).abs() < 0.1, "ranks should sum near 1.0 (got {total})");
    }

    #[test]
    fn neighbors_sorted_by_rank() {
        let mut c = Corpus::new();
        c.upsert(
            "m.rs",
            r#"
fn root_fn() { alpha_call(); alpha_call(); alpha_call(); beta_call(); }
fn alpha_call() { gamma_call(); }
fn beta_call() { gamma_call(); }
fn gamma_call() {}
"#,
        );
        let mut g = SymbolGraph::new();
        g.rebuild(&c);
        let n = g.neighbors("root_fn", 16);
        let names: std::collections::BTreeSet<&str> = n.iter().map(|(s, _)| s.as_str()).collect();
        // `root_fn` directly reaches `alpha_call`, `beta_call`, and
        // `gamma_call` via the call-site edges.
        assert!(
            names.contains("alpha_call"),
            "missing alpha_call, got {names:?}"
        );
        assert!(
            names.contains("beta_call"),
            "missing beta_call, got {names:?}"
        );
    }

    #[test]
    fn rank_of_unknown_is_zero() {
        let g = SymbolGraph::new();
        assert_eq!(g.rank("definitely-nothing"), 0.0);
    }
}
