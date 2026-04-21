//! In-memory corpus: files, chunks, symbol references.
//!
//! A `Corpus` is the single source of truth for the retrieval layer.
//! Chunks are deduped by `(path, start_line, digest)` so re-indexing a
//! file that hasn't changed is free. Paths are normalised to
//! forward-slashed UTF-8 strings at the boundary, since every downstream
//! consumer (MCP tool ids, audit-log records, JSON schemas) is
//! slash-based.

use std::collections::HashMap;

use serde::{Deserialize, Serialize};

use crate::chunk::{Chunk, ChunkKind, Chunker};

/// Opaque identifier for a document in the corpus.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Ord, PartialOrd, Serialize, Deserialize)]
pub struct DocumentId(pub u32);

/// One file-level record.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Document {
    /// Normalised path.
    pub path: String,
    /// Content digest.
    pub digest: String,
    /// Language hint (derived from extension).
    pub lang: LangHint,
    /// Indices into `Corpus::chunks`.
    pub chunk_range: (u32, u32),
}

/// Language hint for routing to appropriate chunker / tree-sitter grammar.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum LangHint {
    /// Rust.
    Rust,
    /// C++ (cpp, cxx, h, hpp).
    Cpp,
    /// Markdown.
    Markdown,
    /// JSON.
    Json,
    /// TOML.
    Toml,
    /// Yaml.
    Yaml,
    /// Everything else.
    Unknown,
}

impl LangHint {
    /// Guess a language from a path.
    pub fn from_path(path: &str) -> Self {
        let lower = path.to_ascii_lowercase();
        if let Some(ext) = std::path::Path::new(&lower).extension().and_then(|e| e.to_str()) {
            match ext {
                "rs" => return Self::Rust,
                "cpp" | "cxx" | "cc" | "h" | "hpp" | "hxx" | "inl" => return Self::Cpp,
                "md" => return Self::Markdown,
                "json" => return Self::Json,
                "toml" => return Self::Toml,
                "yaml" | "yml" => return Self::Yaml,
                _ => {}
            }
        }
        Self::Unknown
    }

    /// True for source code (as opposed to prose / config).
    pub fn is_source(self) -> bool {
        matches!(self, LangHint::Rust | LangHint::Cpp)
    }
}

/// The corpus.
#[derive(Debug, Default)]
pub struct Corpus {
    docs: Vec<Document>,
    chunks: Vec<Chunk>,
    path_index: HashMap<String, DocumentId>,
    chunker: Chunker,
}

impl Corpus {
    /// New empty corpus.
    pub fn new() -> Self {
        Self::default()
    }

    /// Number of documents currently indexed.
    pub fn len(&self) -> usize {
        self.docs.len()
    }

    /// Is the corpus empty?
    pub fn is_empty(&self) -> bool {
        self.docs.is_empty()
    }

    /// Total chunks across every document.
    pub fn chunk_count(&self) -> usize {
        self.chunks.len()
    }

    /// Access the current chunker configuration.
    pub fn chunker(&self) -> &Chunker {
        &self.chunker
    }

    /// Replace the chunker. Does not re-index existing docs; caller
    /// should re-ingest.
    pub fn set_chunker(&mut self, chunker: Chunker) {
        self.chunker = chunker;
    }

    /// Look up a document by path (already-normalised).
    pub fn find(&self, path: &str) -> Option<DocumentId> {
        self.path_index.get(path).copied()
    }

    /// Upsert a document with its body. Returns the new `DocumentId`
    /// and whether content changed.
    pub fn upsert(&mut self, path: impl Into<String>, body: &str) -> (DocumentId, bool) {
        let path = normalise_path(&path.into());
        let digest = blake3::hash(body.as_bytes()).to_hex().to_string();
        let lang = LangHint::from_path(&path);

        // Unchanged fast path.
        if let Some(&id) = self.path_index.get(&path) {
            if self.docs[id.0 as usize].digest == digest {
                return (id, false);
            }
            // Content changed — remove old chunks and re-insert at end.
            let (start, end) = self.docs[id.0 as usize].chunk_range;
            let removed = (end - start) as usize;
            self.chunks.drain(start as usize..end as usize);
            // Shift other docs' chunk ranges.
            for d in &mut self.docs {
                if d.chunk_range.0 >= end {
                    d.chunk_range.0 -= removed as u32;
                    d.chunk_range.1 -= removed as u32;
                }
            }
            let new_start = self.chunks.len() as u32;
            let new_chunks = self.split_chunks(&path, body, lang);
            self.chunks.extend(new_chunks);
            let new_end = self.chunks.len() as u32;
            self.docs[id.0 as usize] = Document {
                path,
                digest,
                lang,
                chunk_range: (new_start, new_end),
            };
            return (id, true);
        }

        // New document.
        let id = DocumentId(self.docs.len() as u32);
        let new_start = self.chunks.len() as u32;
        let new_chunks = self.split_chunks(&path, body, lang);
        self.chunks.extend(new_chunks);
        let new_end = self.chunks.len() as u32;
        self.path_index.insert(path.clone(), id);
        self.docs.push(Document {
            path,
            digest,
            lang,
            chunk_range: (new_start, new_end),
        });
        (id, true)
    }

    /// Remove a document. Returns `true` if it existed.
    pub fn remove(&mut self, path: &str) -> bool {
        let path = normalise_path(path);
        let Some(id) = self.path_index.remove(&path) else {
            return false;
        };
        let (start, end) = self.docs[id.0 as usize].chunk_range;
        let removed = (end - start) as usize;
        self.chunks.drain(start as usize..end as usize);
        // Tombstone the doc (we can't shrink `docs` without invalidating ids).
        self.docs[id.0 as usize] = Document {
            path: String::new(),
            digest: String::new(),
            lang: LangHint::Unknown,
            chunk_range: (0, 0),
        };
        for d in &mut self.docs {
            if d.chunk_range.0 >= end {
                d.chunk_range.0 -= removed as u32;
                d.chunk_range.1 -= removed as u32;
            }
        }
        true
    }

    /// Borrow a document.
    pub fn document(&self, id: DocumentId) -> Option<&Document> {
        self.docs.get(id.0 as usize).filter(|d| !d.path.is_empty())
    }

    /// Iterate every (non-tombstoned) document.
    pub fn documents(&self) -> impl Iterator<Item = (DocumentId, &Document)> {
        self.docs
            .iter()
            .enumerate()
            .filter(|(_, d)| !d.path.is_empty())
            .map(|(i, d)| (DocumentId(i as u32), d))
    }

    /// Get a chunk by global index.
    pub fn chunk(&self, idx: usize) -> Option<&Chunk> {
        self.chunks.get(idx)
    }

    /// Iterate every chunk.
    pub fn chunks(&self) -> impl Iterator<Item = (usize, &Chunk)> {
        self.chunks.iter().enumerate()
    }

    /// Chunks belonging to one document.
    pub fn chunks_for(&self, id: DocumentId) -> impl Iterator<Item = (usize, &Chunk)> {
        let (start, end) = self
            .docs
            .get(id.0 as usize)
            .map(|d| d.chunk_range)
            .unwrap_or((0, 0));
        (start as usize..end as usize).filter_map(move |i| self.chunks.get(i).map(|c| (i, c)))
    }

    /// Dispatch the chunker based on language.
    fn split_chunks(&self, path: &str, body: &str, lang: LangHint) -> Vec<Chunk> {
        let mut out = if lang.is_source() {
            self.chunker.chunk_source(path, body)
        } else {
            self.chunker.chunk_prose(path, body)
        };
        // `Chunker::chunk_*` already populates kind / digest; we just override
        // markdown chunks back to Prose if classify() bumped them somewhere.
        for c in &mut out {
            if !lang.is_source() {
                c.kind = ChunkKind::Prose;
            }
        }
        out
    }
}

fn normalise_path(s: &str) -> String {
    s.replace('\\', "/")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn upsert_inserts_and_indexes() {
        let mut c = Corpus::new();
        let (_id, changed) = c.upsert("foo/bar.md", "# hello\n\nworld paragraph.");
        assert!(changed);
        assert_eq!(c.len(), 1);
        assert!(c.chunk_count() > 0);
    }

    #[test]
    fn upsert_same_content_is_noop() {
        let mut c = Corpus::new();
        c.upsert("foo/bar.md", "# hello\n\nworld.");
        let (_id, changed) = c.upsert("foo/bar.md", "# hello\n\nworld.");
        assert!(!changed);
    }

    #[test]
    fn upsert_replaces_when_content_changes() {
        let mut c = Corpus::new();
        c.upsert("foo/bar.md", "first version text");
        let before = c.chunk_count();
        c.upsert("foo/bar.md", "second version of the file with different text.");
        // Chunk counts may shift but the total doc count is still 1.
        assert_eq!(c.len(), 1);
        assert!(c.chunk_count() > 0);
        let _ = before;
    }

    #[test]
    fn remove_drops_chunks() {
        let mut c = Corpus::new();
        c.upsert("a.md", "content a.");
        c.upsert("b.md", "content b.");
        assert_eq!(c.len(), 2);
        assert!(c.remove("a.md"));
        assert_eq!(c.chunks().count(), c.chunks_for(DocumentId(1)).count());
    }

    #[test]
    fn normalise_path_replaces_backslashes() {
        assert_eq!(normalise_path("a\\b\\c.md"), "a/b/c.md");
    }

    #[test]
    fn lang_from_path_picks_known_ext() {
        assert_eq!(LangHint::from_path("x/y.rs"), LangHint::Rust);
        assert_eq!(LangHint::from_path("x.HPP"), LangHint::Cpp);
        assert_eq!(LangHint::from_path("z.md"), LangHint::Markdown);
        assert_eq!(LangHint::from_path("q.unknown"), LangHint::Unknown);
    }
}
