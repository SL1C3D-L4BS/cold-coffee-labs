//! `bld-rag` — retrieval subsystem (ADR-0013).
//!
//! This crate owns everything BLD needs to build a code + docs corpus,
//! slice it into `Chunk`s, embed/rank them, and answer hybrid retrieval
//! queries. The public API is minimal so the agent loop can depend on it
//! without pulling tree-sitter / sqlite / fastembed into its
//! dependency graph.
//!
//! # Crate layout
//!
//! | Module          | Concern                                                |
//! |-----------------|--------------------------------------------------------|
//! | `chunk`         | Source → `Chunk` splitter (heuristic + tree-sitter).   |
//! | `corpus`        | In-memory corpus: docs, files, IDs, hashes.            |
//! | `bm25`          | Lexical ranker.                                        |
//! | `graph`         | Symbol graph (`petgraph::DiGraph`) + PageRank.         |
//! | `retrieve`      | Hybrid retrieval (BM25 + vector + graph + MMR).        |
//! | `watcher`       | File-watcher hook (Phase 9C/D).                        |
//! | `store` (feat)  | SQLite vector store (sqlite-vec).                      |
//! | `embed` (feat)  | fastembed-rs BGE-small-en adapter.                     |
//!
//! # Non-negotiables (ADR-0013 §2.5)
//!
//! - All state is owned, no `thread_local!`; concurrent callers share an
//!   `Arc<RwLock<Corpus>>`.
//! - Every `Chunk` carries a BLAKE3 content digest so incremental
//!   indexing can skip unchanged files.
//! - Retrieval is deterministic: given the same corpus + seed, results
//!   are stable (important for agent-replay).

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

pub mod bm25;
pub mod chunk;
pub mod corpus;
pub mod graph;
pub mod index;
pub mod retrieve;
pub mod watcher;

pub use chunk::{Chunk, ChunkKind, Chunker};
pub use corpus::{Corpus, DocumentId};
pub use index::{IndexStats, RagIndex};
pub use retrieve::{Hit, RetrievalConfig, RetrievalStrategy};
pub use watcher::{ManualWatcher, WatchEvent, Watcher};
