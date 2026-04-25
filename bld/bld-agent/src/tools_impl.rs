//! Concrete tool implementations backed by `bld-rag`.
//!
//! This module provides a `ToolDispatcher` that wires the first handful
//! of tools in the taxonomy (ADR-0012) to real behaviour. Phase 9C ships
//! the read-only retrieval tools; Wave 9D adds the `build.*`, `code.*`,
//! and `runtime.*` tools that require the editor + C++ bridge.
//!
//! Design notes:
//! - The dispatcher owns a `RagIndex` behind a `Mutex` (the index is
//!   cheap to update and we expect ~1 request at a time per session).
//! - Tool inputs/outputs are JSON — validated at the call boundary so
//!   mis-called tools return a structured `ToolError::BadInput` instead
//!   of panicking.
//! - Every tool is pure, deterministic, and replayable from the audit
//!   log (ADR-0016). No hidden state, no background threads.

use std::path::{Path, PathBuf};
use std::sync::Mutex;

use bld_governance::secret_filter::SecretFilter;
use bld_rag::{RagIndex, RetrievalConfig, RetrievalStrategy};
use bld_tools::registry;
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};

use crate::ToolDispatcher;

/// File loader: maps a relative path into the project into its contents.
pub trait FileLoader: Send + Sync {
    /// Return the file body if it exists and is readable.
    fn load(&self, path: &str) -> Option<String>;

    /// Enumerate files under the project root that match any of the
    /// given glob extensions (e.g. `&["md"]`). Implementors may return
    /// up to an implementation-defined cap.
    fn list(&self, extensions: &[&str]) -> Vec<String>;
}

/// A filesystem-backed loader rooted at a directory.
///
/// Every `load` and `list` call runs paths through `SecretFilter`; anything
/// that matches the built-in deny list (or `.agent_ignore` patterns, when
/// loaded) is silently redacted from results. The agent therefore never sees
/// a `.env` / `.pem` / `.ssh/*` path even if it asks for one by name.
pub struct FsFileLoader {
    /// Project root.
    pub root: PathBuf,
    /// Hard cap on the number of paths `list` returns.
    pub list_cap: usize,
    /// Secret filter applied to every path we surface.
    filter: SecretFilter,
}

impl std::fmt::Debug for FsFileLoader {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("FsFileLoader")
            .field("root", &self.root)
            .field("list_cap", &self.list_cap)
            .finish()
    }
}

impl FsFileLoader {
    /// New loader rooted at `root`.
    pub fn new(root: impl Into<PathBuf>) -> Self {
        let root = root.into();
        let mut filter = SecretFilter::new();
        // Opportunistically load .agent_ignore from the project root.
        if let Ok(body) = std::fs::read_to_string(root.join(".agent_ignore")) {
            let _ = filter.load_agent_ignore(&body);
        }
        Self {
            root,
            list_cap: 4096,
            filter,
        }
    }

    /// Override the secret filter (tests + callers with a pre-baked filter).
    pub fn with_filter(mut self, filter: SecretFilter) -> Self {
        self.filter = filter;
        self
    }

    fn normalise_rel(&self, abs: &Path) -> Option<String> {
        let rel = abs.strip_prefix(&self.root).ok()?;
        Some(rel.to_string_lossy().replace('\\', "/"))
    }

    fn is_blocked(&self, rel: &str) -> bool {
        self.filter.check(Path::new(rel)).is_err()
    }
}

impl FileLoader for FsFileLoader {
    fn load(&self, path: &str) -> Option<String> {
        let norm = path.replace('\\', "/");
        if self.is_blocked(&norm) {
            tracing::debug!(path = %norm, "secret filter blocked load");
            return None;
        }
        let abs = self.root.join(&norm);
        std::fs::read_to_string(abs).ok()
    }

    fn list(&self, extensions: &[&str]) -> Vec<String> {
        fn walk(dir: &Path, out: &mut Vec<PathBuf>, cap: usize) {
            if out.len() >= cap {
                return;
            }
            let Ok(entries) = std::fs::read_dir(dir) else { return };
            for entry in entries.flatten() {
                if out.len() >= cap {
                    return;
                }
                let p = entry.path();
                if p.is_dir() {
                    // Skip obvious build / vendor directories.
                    let skip = p
                        .file_name()
                        .and_then(|n| n.to_str())
                        .map(|n| {
                            matches!(
                                n,
                                ".git" | "target" | "node_modules" | "build" | ".cache"
                            )
                        })
                        .unwrap_or(false);
                    if !skip {
                        walk(&p, out, cap);
                    }
                } else {
                    out.push(p);
                }
            }
        }
        let mut all: Vec<PathBuf> = Vec::new();
        walk(&self.root, &mut all, self.list_cap);
        all.into_iter()
            .filter(|p| {
                let Some(ext) = p.extension().and_then(|e| e.to_str()) else {
                    return false;
                };
                extensions.iter().any(|want| want.eq_ignore_ascii_case(ext))
            })
            .filter_map(|p| self.normalise_rel(&p))
            .filter(|rel| !self.is_blocked(rel))
            .collect()
    }
}

/// RAG-backed tool dispatcher. Implements `ToolDispatcher` for the
/// read-only subset of Phase 9 tools.
pub struct RagDispatcher {
    index: Mutex<RagIndex>,
    loader: Box<dyn FileLoader>,
}

impl std::fmt::Debug for RagDispatcher {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RagDispatcher").finish()
    }
}

/// Search parameters for `docs.search`.
#[derive(Debug, Deserialize)]
struct DocsSearchArgs {
    query: String,
    #[serde(default)]
    k: Option<usize>,
    #[serde(default)]
    strategy: Option<String>,
}

/// Parameters for `project.list_scenes` / `docs.list`.
#[derive(Debug, Default, Deserialize)]
struct ListArgs {
    #[serde(default)]
    extensions: Option<Vec<String>>,
}

/// Parameters for `docs.lookup`.
#[derive(Debug, Deserialize)]
struct DocsLookupArgs {
    path: String,
    #[serde(default)]
    start_line: Option<u32>,
    #[serde(default)]
    end_line: Option<u32>,
}

/// Parameters for `rag.index_file`.
#[derive(Debug, Deserialize)]
struct IndexFileArgs {
    path: String,
    #[serde(default)]
    body: Option<String>,
}

/// Exported serialisable hit for JSON responses.
#[derive(Debug, Serialize)]
struct HitJson {
    path: String,
    lines: (u32, u32),
    score: f32,
    snippet: String,
}

impl RagDispatcher {
    /// New dispatcher with the given loader.
    pub fn new(loader: Box<dyn FileLoader>) -> Self {
        Self {
            index: Mutex::new(RagIndex::new()),
            loader,
        }
    }

    /// Directly ingest a file (test + bootstrap helper).
    pub fn ingest(&self, path: impl Into<String>, body: &str) {
        #[allow(clippy::expect_used)]
        let mut idx = self.index.lock().expect("index mutex poisoned");
        idx.upsert(path, body);
    }

    /// Pre-warm the index by scanning the loader's project tree.
    pub fn bootstrap_from_disk(&self, extensions: &[&str]) -> usize {
        let paths = self.loader.list(extensions);
        #[allow(clippy::expect_used)]
        let mut idx = self.index.lock().expect("index mutex poisoned");
        let mut count = 0;
        for p in paths {
            if let Some(body) = self.loader.load(&p) {
                if idx.upsert(&p, &body) {
                    count += 1;
                }
            }
        }
        count
    }

    fn docs_search(&self, args: &Value) -> Result<Value, String> {
        let a: DocsSearchArgs =
            serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
        let strategy = match a.strategy.as_deref() {
            Some("lexical") => RetrievalStrategy::Lexical,
            Some("semantic") => RetrievalStrategy::Semantic,
            _ => RetrievalStrategy::Hybrid,
        };
        let cfg = RetrievalConfig {
            k: a.k.unwrap_or(8).clamp(1, 32),
            strategy,
            ..Default::default()
        };
        #[allow(clippy::expect_used)]
        let mut idx = self.index.lock().expect("index mutex poisoned");
        let hits = idx.retrieve(&a.query, Some(&cfg));
        let mapped: Vec<HitJson> = hits
            .into_iter()
            .map(|h| HitJson {
                path: h.path,
                lines: h.lines,
                score: h.score,
                snippet: truncate(&h.body, 600),
            })
            .collect();
        Ok(json!({
            "query": a.query,
            "k": cfg.k,
            "strategy": format!("{:?}", cfg.strategy).to_lowercase(),
            "hits": mapped,
        }))
    }

    fn rag_query(&self, args: &Value) -> Result<Value, String> {
        // rag.query has a richer breakdown in the response so the model can
        // see why a hit was chosen; identical inputs otherwise.
        let a: DocsSearchArgs =
            serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
        let strategy = match a.strategy.as_deref() {
            Some("lexical") => RetrievalStrategy::Lexical,
            Some("semantic") => RetrievalStrategy::Semantic,
            _ => RetrievalStrategy::Hybrid,
        };
        let cfg = RetrievalConfig {
            k: a.k.unwrap_or(8).clamp(1, 32),
            strategy,
            ..Default::default()
        };
        #[allow(clippy::expect_used)]
        let mut idx = self.index.lock().expect("index mutex poisoned");
        let hits = idx.retrieve(&a.query, Some(&cfg));
        let mapped: Vec<Value> = hits
            .into_iter()
            .map(|h| {
                json!({
                    "path": h.path,
                    "lines": [h.lines.0, h.lines.1],
                    "score": h.score,
                    "breakdown": {
                        "bm25": h.breakdown.bm25,
                        "graph": h.breakdown.graph,
                        "vector": h.breakdown.vector,
                    },
                    "snippet": truncate(&h.body, 600),
                })
            })
            .collect();
        Ok(json!({
            "query": a.query,
            "k": cfg.k,
            "strategy": format!("{:?}", cfg.strategy).to_lowercase(),
            "hits": mapped,
        }))
    }

    fn rag_graph_neighbors(&self, args: &Value) -> Result<Value, String> {
        #[derive(Debug, Deserialize)]
        struct A {
            symbol: String,
            #[serde(default)]
            limit: Option<usize>,
        }
        let a: A = serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
        #[allow(clippy::expect_used)]
        let mut idx = self.index.lock().expect("index mutex poisoned");
        idx.rebuild_if_dirty();
        let neighbors = idx
            .symbol_graph()
            .neighbors(&a.symbol, a.limit.unwrap_or(16));
        let mapped: Vec<Value> = neighbors
            .into_iter()
            .map(|(s, r)| json!({"symbol": s, "rank": r}))
            .collect();
        Ok(json!({
            "symbol": a.symbol,
            "neighbors": mapped,
        }))
    }

    fn docs_open(&self, args: &Value) -> Result<Value, String> {
        let a: DocsLookupArgs =
            serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
        // Hot path: first try the already-indexed corpus.
        #[allow(clippy::expect_used)]
        let idx = self.index.lock().expect("index mutex poisoned");
        if let Some(doc_id) = idx.corpus().find(&a.path.replace('\\', "/")) {
            let chunks: Vec<_> = idx.corpus().chunks_for(doc_id).collect();
            if !chunks.is_empty() {
                let body: String = chunks.iter().map(|(_, c)| c.body.clone()).collect();
                let sliced = slice_by_lines(&body, a.start_line, a.end_line);
                return Ok(json!({
                    "path": a.path,
                    "lines": [a.start_line.unwrap_or(1), a.end_line.unwrap_or(0)],
                    "body": sliced,
                }));
            }
        }
        drop(idx);
        // Fall back to the loader (not yet indexed).
        let body = self
            .loader
            .load(&a.path)
            .ok_or_else(|| format!("file not found: {}", a.path))?;
        let sliced = slice_by_lines(&body, a.start_line, a.end_line);
        Ok(json!({
            "path": a.path,
            "lines": [a.start_line.unwrap_or(1), a.end_line.unwrap_or(0)],
            "body": sliced,
        }))
    }

    fn project_list_scenes(&self, args: &Value) -> Result<Value, String> {
        let a: ListArgs = serde_json::from_value(args.clone()).unwrap_or_default();
        let exts_owned: Vec<String> = a
            .extensions
            .unwrap_or_else(|| vec!["scene".to_owned(), "gwscene".to_owned()]);
        let exts: Vec<&str> = exts_owned.iter().map(String::as_str).collect();
        let paths = self.loader.list(&exts);
        Ok(json!({ "count": paths.len(), "paths": paths }))
    }

    fn rag_index_file(&self, args: &Value) -> Result<Value, String> {
        let a: IndexFileArgs =
            serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
        let body_owned;
        let body = match a.body.as_deref() {
            Some(b) => b,
            None => {
                body_owned = self
                    .loader
                    .load(&a.path)
                    .ok_or_else(|| format!("file not found: {}", a.path))?;
                body_owned.as_str()
            }
        };
        #[allow(clippy::expect_used)]
        let mut idx = self.index.lock().expect("index mutex poisoned");
        let changed = idx.upsert(&a.path, body);
        Ok(json!({
            "path": a.path,
            "changed": changed,
            "doc_count": idx.doc_count(),
            "chunk_count": idx.chunk_count(),
        }))
    }

}

impl ToolDispatcher for RagDispatcher {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        if let Some(entry) = registry::find_dispatch(tool_id) {
            return (entry.dispatch)(args);
        }
        // Sacrilege copilot tools: descriptors always ship; host-backed bodies
        // land in later phases. Return structured JSON so MCP/agents can branch
        // without a generic "not handled" string.
        if tool_id.starts_with("sacrilege.") {
            return Ok(json!({
                "status": "deferred",
                "tool": tool_id,
                "message": "Sacrilege execution path is not installed in this dispatcher build."
            }));
        }
        match tool_id {
            "docs.search" => self.docs_search(args),
            "docs.open" => self.docs_open(args),
            "project.list_scenes" => self.project_list_scenes(args),
            "rag.index_file" => self.rag_index_file(args),
            "rag.query" => self.rag_query(args),
            "rag.graph_neighbors" => self.rag_graph_neighbors(args),
            other => Err(format!("tool not handled by rag dispatcher: {other}")),
        }
    }
}

fn truncate(s: &str, max: usize) -> String {
    if s.len() <= max {
        s.to_owned()
    } else {
        let mut cut = max;
        while !s.is_char_boundary(cut) && cut > 0 {
            cut -= 1;
        }
        let mut out = String::with_capacity(cut + 1);
        out.push_str(&s[..cut]);
        out.push('…');
        out
    }
}

fn slice_by_lines(body: &str, start: Option<u32>, end: Option<u32>) -> String {
    let start = start.unwrap_or(1).max(1) as usize;
    let lines: Vec<&str> = body.lines().collect();
    let end = match end {
        Some(e) if e as usize >= start => (e as usize).min(lines.len()),
        _ => lines.len(),
    };
    if start > lines.len() {
        return String::new();
    }
    lines[start - 1..end].join("\n")
}

#[cfg(test)]
mod tests {
    use super::*;

    /// In-memory loader for deterministic tests.
    #[derive(Debug, Default)]
    struct MemLoader {
        files: Vec<(String, String)>,
    }

    impl MemLoader {
        fn insert(&mut self, path: &str, body: &str) {
            self.files.push((path.to_owned(), body.to_owned()));
        }
    }

    impl FileLoader for MemLoader {
        fn load(&self, path: &str) -> Option<String> {
            self.files
                .iter()
                .find(|(p, _)| p == path)
                .map(|(_, b)| b.clone())
        }
        fn list(&self, exts: &[&str]) -> Vec<String> {
            self.files
                .iter()
                .filter(|(p, _)| {
                    Path::new(p)
                        .extension()
                        .and_then(|e| e.to_str())
                        .map(|e| exts.iter().any(|want| want.eq_ignore_ascii_case(e)))
                        .unwrap_or(false)
                })
                .map(|(p, _)| p.clone())
                .collect()
        }
    }

    fn loader_with(files: &[(&str, &str)]) -> Box<dyn FileLoader> {
        let mut l = MemLoader::default();
        for (p, b) in files {
            l.insert(p, b);
        }
        Box::new(l)
    }

    #[test]
    fn docs_search_returns_hits_after_ingest() {
        let d = RagDispatcher::new(loader_with(&[]));
        d.ingest(
            "guide.md",
            "The BLD copilot uses hybrid retrieval over indexed docs.\n\n\
             It respects the CommandStack for every mutation.",
        );
        d.ingest("other.md", "Nothing relevant here at all, friend.");
        let out = d
            .dispatch("docs.search", &json!({"query": "copilot", "k": 5}))
            .unwrap();
        let hits = out.get("hits").unwrap().as_array().unwrap();
        assert!(!hits.is_empty());
        assert_eq!(hits[0].get("path").unwrap(), "guide.md");
    }

    #[test]
    fn docs_open_slices_by_line() {
        let d = RagDispatcher::new(loader_with(&[(
            "notes.md",
            "line one\nline two\nline three\nline four",
        )]));
        let out = d
            .dispatch(
                "docs.open",
                &json!({"path": "notes.md", "start_line": 2, "end_line": 3}),
            )
            .unwrap();
        let body = out.get("body").unwrap().as_str().unwrap();
        assert_eq!(body, "line two\nline three");
    }

    #[test]
    fn project_list_scenes_filters_by_extension() {
        let d = RagDispatcher::new(loader_with(&[
            ("scenes/a.scene", ""),
            ("scenes/b.scene", ""),
            ("src/main.rs", ""),
        ]));
        let out = d
            .dispatch("project.list_scenes", &json!({}))
            .unwrap();
        assert_eq!(out.get("count").unwrap(), 2);
    }

    #[test]
    fn rag_index_file_ingests_and_increments() {
        let d = RagDispatcher::new(loader_with(&[(
            "r.md",
            "first version of the document with enough text to chunk.",
        )]));
        let out = d
            .dispatch("rag.index_file", &json!({"path": "r.md"}))
            .unwrap();
        assert_eq!(out.get("changed").unwrap(), true);
        assert_eq!(out.get("doc_count").unwrap(), 1);
    }

    #[test]
    fn rag_query_returns_breakdown() {
        let d = RagDispatcher::new(loader_with(&[]));
        d.ingest(
            "engine.md",
            "The renderer draws every frame. The command stack manages undo.",
        );
        let out = d
            .dispatch("rag.query", &json!({"query": "renderer", "k": 3}))
            .unwrap();
        let hits = out.get("hits").unwrap().as_array().unwrap();
        assert!(!hits.is_empty());
        assert!(hits[0].get("breakdown").is_some());
    }

    #[test]
    fn rag_graph_neighbors_returns_list() {
        let d = RagDispatcher::new(loader_with(&[]));
        d.ingest(
            "lib.rs",
            "fn alpha_proc() { beta_proc(); }\nfn beta_proc() { gamma_proc(); }\nfn gamma_proc() {}\n",
        );
        let out = d
            .dispatch(
                "rag.graph_neighbors",
                &json!({"symbol": "alpha_proc", "limit": 5}),
            )
            .unwrap();
        let neighbors = out.get("neighbors").unwrap().as_array().unwrap();
        assert!(!neighbors.is_empty());
    }

    #[test]
    fn fs_loader_secret_filter_blocks_dotenv() {
        let tmp = tempfile::tempdir().unwrap();
        let root = tmp.path();
        std::fs::write(root.join(".env"), "SECRET=shh").unwrap();
        std::fs::write(root.join("readme.md"), "hi").unwrap();
        let loader = FsFileLoader::new(root);
        let listed = loader.list(&["env", "md"]);
        assert!(listed.iter().any(|p| p == "readme.md"));
        assert!(
            !listed.iter().any(|p| p == ".env"),
            "secret filter must hide .env"
        );
        // Direct load must also be blocked.
        assert!(loader.load(".env").is_none());
    }

    #[test]
    fn unknown_tool_errors_with_clear_message() {
        let d = RagDispatcher::new(loader_with(&[]));
        let err = d.dispatch("does.not.exist", &json!({})).unwrap_err();
        assert!(err.contains("not handled"));
    }

    #[test]
    fn sacrilege_tools_return_deferred_json() {
        let d = RagDispatcher::new(loader_with(&[]));
        let out = d
            .dispatch("sacrilege.exploit_detect", &json!({}))
            .unwrap();
        assert_eq!(out.get("status").unwrap(), "deferred");
        assert_eq!(
            out.get("tool").unwrap().as_str().unwrap(),
            "sacrilege.exploit_detect"
        );
    }
}
