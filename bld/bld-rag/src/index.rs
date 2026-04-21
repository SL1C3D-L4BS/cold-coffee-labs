//! High-level retrieval index.
//!
//! `RagIndex` is the single entry-point Phase 9 tools interact with. It
//! owns a `Corpus`, a `Bm25Index`, and a `SymbolGraph`, and keeps them
//! coherent with a tiny "dirty bit" so derived structures are only
//! rebuilt when the corpus actually changed. Higher layers do not need
//! to know about any of the internals — they just call
//! `upsert` / `remove` / `apply_events` / `retrieve`.
//!
//! The orchestrator is intentionally synchronous. The file-watcher in
//! `watcher` pushes events into a buffer, and whatever is driving the
//! agent loop drains them on its cadence (typically once per turn).
//! That gives us a deterministic replay model: the index state is a
//! pure function of the sequence of events applied to it.

use std::collections::HashSet;

use crate::bm25::Bm25Index;
use crate::corpus::Corpus;
use crate::graph::SymbolGraph;
use crate::retrieve::{retrieve, Hit, RetrievalConfig};
use crate::watcher::WatchEvent;

/// Summary of a bulk ingest / refresh.
#[derive(Debug, Default, Clone, Copy, PartialEq, Eq)]
pub struct IndexStats {
    /// New documents added.
    pub added: u32,
    /// Existing documents whose content changed.
    pub updated: u32,
    /// Documents removed.
    pub removed: u32,
    /// No-op (content identical).
    pub unchanged: u32,
}

impl IndexStats {
    /// True if any mutating change happened.
    pub fn has_changes(&self) -> bool {
        self.added + self.updated + self.removed > 0
    }
}

/// High-level retrieval index.
#[derive(Debug)]
pub struct RagIndex {
    corpus: Corpus,
    bm25: Bm25Index,
    graph: SymbolGraph,
    /// Set when the corpus mutated — next `retrieve` will rebuild.
    dirty: bool,
    /// Default retrieval config applied when callers don't pass one.
    default_cfg: RetrievalConfig,
}

impl Default for RagIndex {
    fn default() -> Self {
        Self::new()
    }
}

impl RagIndex {
    /// Fresh empty index.
    pub fn new() -> Self {
        Self {
            corpus: Corpus::new(),
            bm25: Bm25Index::default(),
            graph: SymbolGraph::new(),
            dirty: false,
            default_cfg: RetrievalConfig::default(),
        }
    }

    /// Borrow the underlying corpus (read-only).
    pub fn corpus(&self) -> &Corpus {
        &self.corpus
    }

    /// Borrow the BM25 index (for introspection / tests).
    pub fn bm25(&self) -> &Bm25Index {
        &self.bm25
    }

    /// Borrow the symbol graph (for introspection / tests).
    pub fn symbol_graph(&self) -> &SymbolGraph {
        &self.graph
    }

    /// Override the default retrieval config.
    pub fn set_default_config(&mut self, cfg: RetrievalConfig) {
        self.default_cfg = cfg;
    }

    /// Number of indexed documents.
    pub fn doc_count(&self) -> usize {
        self.corpus.len()
    }

    /// Number of chunks across every document.
    pub fn chunk_count(&self) -> usize {
        self.corpus.chunk_count()
    }

    /// True if derived indexes need a rebuild before the next query.
    pub fn is_dirty(&self) -> bool {
        self.dirty
    }

    /// Ingest (or refresh) a single file.
    pub fn upsert(&mut self, path: impl Into<String>, body: &str) -> bool {
        let (_, changed) = self.corpus.upsert(path, body);
        if changed {
            self.dirty = true;
        }
        changed
    }

    /// Remove one file.
    pub fn remove(&mut self, path: &str) -> bool {
        let removed = self.corpus.remove(path);
        if removed {
            self.dirty = true;
        }
        removed
    }

    /// Apply a batch of watcher events; returns summary stats.
    pub fn apply_events<I>(&mut self, events: I, loader: &mut dyn FnMut(&str) -> Option<String>) -> IndexStats
    where
        I: IntoIterator<Item = WatchEvent>,
    {
        let mut stats = IndexStats::default();
        let mut touched: HashSet<String> = HashSet::new();
        for ev in events {
            match ev {
                WatchEvent::Upsert(p) => {
                    let path = p.to_string_lossy().replace('\\', "/");
                    if touched.contains(&path) {
                        stats.unchanged += 1;
                        continue;
                    }
                    let Some(body) = loader(&path) else {
                        continue;
                    };
                    let had = self.corpus.find(&path).is_some();
                    if self.upsert(&path, &body) {
                        if had {
                            stats.updated += 1;
                        } else {
                            stats.added += 1;
                        }
                    } else {
                        stats.unchanged += 1;
                    }
                    touched.insert(path);
                }
                WatchEvent::Remove(p) => {
                    let path = p.to_string_lossy().replace('\\', "/");
                    if self.remove(&path) {
                        stats.removed += 1;
                    }
                }
            }
        }
        stats
    }

    /// Rebuild BM25 + symbol graph if the corpus is dirty.
    pub fn rebuild_if_dirty(&mut self) {
        if !self.dirty {
            return;
        }
        self.bm25 = Bm25Index::build(&self.corpus);
        self.graph.rebuild(&self.corpus);
        self.dirty = false;
    }

    /// Run a hybrid retrieval query. Rebuilds derived indexes on demand.
    pub fn retrieve(&mut self, query: &str, cfg: Option<&RetrievalConfig>) -> Vec<Hit> {
        self.rebuild_if_dirty();
        let cfg = cfg.unwrap_or(&self.default_cfg);
        retrieve(&self.corpus, &self.bm25, &self.graph, query, cfg)
    }

    /// Non-mutating retrieve (errors if rebuild is needed). Handy for
    /// read-only call sites that want to prove the index is clean.
    pub fn retrieve_strict(&self, query: &str, cfg: Option<&RetrievalConfig>) -> Option<Vec<Hit>> {
        if self.dirty {
            return None;
        }
        let cfg = cfg.unwrap_or(&self.default_cfg);
        Some(retrieve(&self.corpus, &self.bm25, &self.graph, query, cfg))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::watcher::WatchEvent;
    use std::path::PathBuf;

    #[test]
    fn upsert_marks_dirty_then_retrieval_rebuilds() {
        let mut idx = RagIndex::new();
        idx.upsert("a.md", "hello world\n\nsome more text about hello.");
        idx.upsert("b.md", "entirely different content about lemons.");
        assert!(idx.is_dirty());
        let hits = idx.retrieve("hello", None);
        assert!(!idx.is_dirty());
        assert!(!hits.is_empty());
        assert!(hits[0].body.to_lowercase().contains("hello"));
    }

    #[test]
    fn remove_unindexes_document() {
        let mut idx = RagIndex::new();
        idx.upsert("a.md", "alpha content about foo.");
        idx.upsert("b.md", "beta content about foo.");
        let before = idx.retrieve("foo", None).len();
        assert!(before >= 2);
        assert!(idx.remove("a.md"));
        let after = idx.retrieve("foo", None).len();
        assert!(after < before, "after remove should yield fewer hits");
    }

    #[test]
    fn apply_events_routes_through_loader() {
        let mut idx = RagIndex::new();
        let events = vec![
            WatchEvent::Upsert(PathBuf::from("docs/x.md")),
            WatchEvent::Upsert(PathBuf::from("docs/y.md")),
            WatchEvent::Remove(PathBuf::from("docs/z.md")),
        ];
        let mut loader = |p: &str| -> Option<String> {
            match p {
                "docs/x.md" => Some("x content with apples".to_owned()),
                "docs/y.md" => Some("y content with bananas".to_owned()),
                _ => None,
            }
        };
        let stats = idx.apply_events(events, &mut loader);
        assert_eq!(stats.added, 2);
        assert_eq!(stats.updated, 0);
        assert_eq!(stats.removed, 0); // z.md was never indexed
        assert_eq!(idx.doc_count(), 2);
    }

    #[test]
    fn strict_retrieve_requires_clean_index() {
        let mut idx = RagIndex::new();
        idx.upsert("a.md", "content");
        assert!(idx.retrieve_strict("content", None).is_none());
        idx.rebuild_if_dirty();
        assert!(idx.retrieve_strict("content", None).is_some());
    }
}
