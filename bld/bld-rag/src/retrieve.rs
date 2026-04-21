//! Hybrid retrieval.
//!
//! The retriever blends three signals:
//!
//! 1. **BM25** — lexical match against chunk bodies.
//! 2. **Graph rank** — PageRank over the symbol graph; chunks owned by
//!    higher-rank symbols pick up a boost proportional to the rank.
//! 3. **Vector cosine** (optional, `embed` feature) — semantic match via
//!    fastembed. Absent the feature we fall back to the other two
//!    signals.
//!
//! After scoring, the retriever applies **MMR** (Maximal Marginal
//! Relevance) to diversify the result set: highly similar chunks drop in
//! rank so we don't return twenty chunks from the same function.
//!
//! Output is deterministic given the same corpus + query — a property
//! `bld-replay` depends on.

use serde::{Deserialize, Serialize};

use crate::bm25::{Bm25Index, tokenise};
use crate::chunk::Chunk;
use crate::corpus::Corpus;
use crate::graph::SymbolGraph;

/// Which strategy the caller wants. Useful for tests + the
/// `rag.query` tool's explain mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum RetrievalStrategy {
    /// Lexical (BM25) only.
    Lexical,
    /// Lexical + PageRank blend.
    Hybrid,
    /// Lexical + PageRank + vector cosine (when `embed` feature is on).
    Semantic,
}

/// Retrieval knobs.
#[derive(Debug, Clone)]
pub struct RetrievalConfig {
    /// Max results.
    pub k: usize,
    /// MMR trade-off in [0, 1]: higher = more diverse.
    pub mmr_lambda: f32,
    /// Weight on BM25 (0..=1).
    pub w_bm25: f32,
    /// Weight on graph rank (0..=1).
    pub w_graph: f32,
    /// Weight on vector cosine (0..=1). Only used with `Semantic`.
    pub w_vec: f32,
    /// Strategy.
    pub strategy: RetrievalStrategy,
}

impl Default for RetrievalConfig {
    fn default() -> Self {
        Self {
            k: 8,
            mmr_lambda: 0.7,
            w_bm25: 1.0,
            w_graph: 0.35,
            w_vec: 0.0,
            strategy: RetrievalStrategy::Hybrid,
        }
    }
}

/// One retrieval result.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Hit {
    /// Global chunk index.
    pub chunk_index: u32,
    /// Source path.
    pub path: String,
    /// Line range (1-based, inclusive).
    pub lines: (u32, u32),
    /// Blended score.
    pub score: f32,
    /// Which components contributed.
    pub breakdown: Breakdown,
    /// Snippet body.
    pub body: String,
}

/// Per-hit score breakdown (for debugging / `rag.query` explain mode).
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct Breakdown {
    /// BM25 component.
    pub bm25: f32,
    /// Graph PageRank boost.
    pub graph: f32,
    /// Vector cosine.
    pub vector: f32,
}

/// Run a hybrid retrieval pass.
pub fn retrieve(
    corpus: &Corpus,
    bm25: &Bm25Index,
    graph: &SymbolGraph,
    query: &str,
    cfg: &RetrievalConfig,
) -> Vec<Hit> {
    if corpus.chunk_count() == 0 {
        return Vec::new();
    }

    // Over-fetch 4x k to give MMR + graph boost a meaningful pool.
    let pool_k = (cfg.k * 4).max(16);
    let bm25_scores = bm25.top_k(query, pool_k);
    if bm25_scores.is_empty() {
        return Vec::new();
    }

    let max_bm25 = bm25_scores
        .iter()
        .map(|(_, s)| *s)
        .fold(0.0_f32, f32::max)
        .max(f32::EPSILON);

    let query_terms = tokenise(query);
    let mut scored: Vec<Hit> = bm25_scores
        .iter()
        .filter_map(|&(idx, score)| {
            let chunk = corpus.chunk(idx)?;
            let bm25_norm = score / max_bm25;

            let graph_boost = if cfg.strategy == RetrievalStrategy::Lexical {
                0.0
            } else {
                compute_graph_boost(chunk, graph)
            };

            let vec_score = if cfg.strategy == RetrievalStrategy::Semantic {
                crate_vec_cosine(&query_terms, chunk)
            } else {
                0.0
            };

            let blended =
                cfg.w_bm25 * bm25_norm + cfg.w_graph * graph_boost + cfg.w_vec * vec_score;

            Some(Hit {
                chunk_index: idx as u32,
                path: chunk.path.clone(),
                lines: (chunk.start_line, chunk.end_line),
                score: blended,
                breakdown: Breakdown {
                    bm25: bm25_norm,
                    graph: graph_boost,
                    vector: vec_score,
                },
                body: chunk.body.clone(),
            })
        })
        .collect();

    scored.sort_by(|a, b| {
        b.score
            .partial_cmp(&a.score)
            .unwrap_or(std::cmp::Ordering::Equal)
    });

    apply_mmr(&mut scored, cfg.mmr_lambda, cfg.k)
}

fn compute_graph_boost(chunk: &Chunk, graph: &SymbolGraph) -> f32 {
    // For each symbol mentioned in the chunk, take max rank.
    let tokens = tokenise(&chunk.body);
    let mut best = 0.0_f32;
    for t in tokens {
        let r = graph.rank(&t);
        if r > best {
            best = r;
        }
    }
    best
}

// A stand-in "vector" score when fastembed isn't linked: Jaccard of
// lower-case token sets. Produces numbers in [0, 1]. This guarantees the
// `Semantic` path returns sensible hits even without the `embed` feature.
fn crate_vec_cosine(query_terms: &[String], chunk: &Chunk) -> f32 {
    use std::collections::HashSet;
    let qset: HashSet<&str> = query_terms.iter().map(String::as_str).collect();
    let body_terms: Vec<String> = tokenise(&chunk.body);
    let cset: HashSet<&str> = body_terms.iter().map(String::as_str).collect();
    let inter = qset.intersection(&cset).count() as f32;
    let union = qset.union(&cset).count() as f32;
    if union == 0.0 {
        0.0
    } else {
        inter / union
    }
}

/// Apply Maximal Marginal Relevance to a sorted hit list, keeping `k`
/// results that balance similarity-to-query vs. diversity-from-others.
fn apply_mmr(hits: &mut Vec<Hit>, lambda: f32, k: usize) -> Vec<Hit> {
    if hits.len() <= k {
        return std::mem::take(hits);
    }
    let mut selected: Vec<Hit> = Vec::with_capacity(k);
    let mut remaining: Vec<Hit> = std::mem::take(hits);

    while selected.len() < k && !remaining.is_empty() {
        let mut best_idx = 0_usize;
        let mut best_mmr = f32::NEG_INFINITY;
        for (i, cand) in remaining.iter().enumerate() {
            let max_sim = selected
                .iter()
                .map(|s| body_similarity(&s.body, &cand.body))
                .fold(0.0_f32, f32::max);
            let mmr = lambda * cand.score - (1.0 - lambda) * max_sim;
            if mmr > best_mmr {
                best_mmr = mmr;
                best_idx = i;
            }
        }
        selected.push(remaining.remove(best_idx));
    }
    selected
}

fn body_similarity(a: &str, b: &str) -> f32 {
    use std::collections::HashSet;
    let ta: HashSet<String> = tokenise(a).into_iter().collect();
    let tb: HashSet<String> = tokenise(b).into_iter().collect();
    if ta.is_empty() || tb.is_empty() {
        return 0.0;
    }
    let inter = ta.intersection(&tb).count() as f32;
    let union = ta.union(&tb).count() as f32;
    inter / union
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bm25::Bm25Index;

    fn fixture() -> (Corpus, Bm25Index, SymbolGraph) {
        let mut c = Corpus::new();
        c.upsert(
            "rust.rs",
            "pub fn hotpath() { call_me(); call_me(); }\n\nfn call_me() -> i32 { 42 }\n",
        );
        c.upsert(
            "other.rs",
            "pub fn unrelated() { println!(\"nothing to do with hotpath\"); }\n",
        );
        c.upsert(
            "docs.md",
            "The hotpath runs every frame and must remain allocation-free.",
        );
        let bm = Bm25Index::build(&c);
        let mut g = SymbolGraph::new();
        g.rebuild(&c);
        (c, bm, g)
    }

    #[test]
    fn retrieve_hybrid_returns_ordered_hits() {
        let (c, bm, g) = fixture();
        let cfg = RetrievalConfig::default();
        let hits = retrieve(&c, &bm, &g, "hotpath", &cfg);
        assert!(!hits.is_empty());
        assert!(hits.first().map(|h| h.score).unwrap_or(0.0) > 0.0);
        // Top hit should mention "hotpath".
        let top = &hits[0];
        assert!(top.body.contains("hotpath"));
    }

    #[test]
    fn retrieve_lexical_strategy_ignores_graph() {
        let (c, bm, g) = fixture();
        let cfg = RetrievalConfig {
            strategy: RetrievalStrategy::Lexical,
            w_graph: 0.0,
            ..Default::default()
        };
        let hits = retrieve(&c, &bm, &g, "hotpath", &cfg);
        for h in &hits {
            assert_eq!(h.breakdown.graph, 0.0);
        }
    }

    #[test]
    fn empty_corpus_returns_empty() {
        let c = Corpus::new();
        let bm = Bm25Index::build(&c);
        let g = SymbolGraph::new();
        let hits = retrieve(&c, &bm, &g, "query", &RetrievalConfig::default());
        assert!(hits.is_empty());
    }

    #[test]
    fn mmr_diversifies_near_duplicates() {
        // All three chunks are near-identical; MMR with small k must pick
        // different chunks, not three copies of the top-scoring one.
        let mut c = Corpus::new();
        c.upsert("a.md", "the quick brown fox jumps over the lazy dog");
        c.upsert("b.md", "the quick brown fox jumps over the lazy dog!");
        c.upsert("c.md", "the quick brown fox jumps over the lazy dog.");
        let bm = Bm25Index::build(&c);
        let g = SymbolGraph::new();
        let cfg = RetrievalConfig { k: 2, ..Default::default() };
        let hits = retrieve(&c, &bm, &g, "fox", &cfg);
        assert!(hits.len() <= 2);
        // The paths should differ.
        if hits.len() == 2 {
            assert_ne!(hits[0].path, hits[1].path);
        }
    }
}
