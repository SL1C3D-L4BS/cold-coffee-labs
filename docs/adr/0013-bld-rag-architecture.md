# ADR 0013 — BLD RAG architecture (two-index, hybrid retrieval, sqlite-vec)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 9 wave 9C opening)
- **Deciders:** Cold Coffee Labs engine group (Claude Opus 4.7 drafting; founder final)
- **Supersedes:** The `sqlite-vss` decision in `docs/02_SYSTEMS_AND_SIMULATIONS.md §13` (superseded here — `sqlite-vss` is deprecated upstream) and the single-model `Nomic Embed Code` decision (split into two-path design for VRAM-budget compliance).
- **Superseded by:** —
- **Related:** `CLAUDE.md` #1 (RX 580 budget), #16 (determinism); `docs/00_SPECIFICATION_Claude_Opus_4.7.md` §2.5; `docs/02_SYSTEMS_AND_SIMULATIONS.md §13`; `docs/04_LANGUAGE_RESEARCH_GUIDE.md §3.5`; ADR-0010 (provider abstraction — embedding goes through it).

## 1. Context

Wave 9C (weeks 052–053) stands up BLD's RAG subsystem. Design goals:

- Top-10 hit on any "where is X used?" query in < 200 ms P95 on a ~50 k-chunk engine-docs index.
- Fit the RX 580 8 GB VRAM budget while indexing (non-negotiable #1).
- Deterministic: same source tree → same embedding rows (up to model nondeterminism, which is pinned).
- Incremental: 1-file edit → 1-file re-embed, not whole-tree.
- **Two indices:** engine-docs index (ships prebuilt as a blob) and user-project index (built on first run, updated on save).
- Rust-only; no Python sidecar.

### 1.1 Research deltas since docs were written

- `sqlite-vss` has been **deprecated** by its original author. The successor is **`sqlite-vec`** (pure C, no Faiss dep, int8/binary/float vectors, multi-platform, loaded as a SQLite extension).
- **Nomic Embed Code** — the model `docs/02 §13` originally pinned for embeddings — is **7 B params / 26.35 GB**. Does not fit on an RX 580 at runtime. It remains the correct choice for the **prebuilt** engine-docs index (built on developer boxes with adequate VRAM), but the user-project runtime path needs a smaller model: `nomic-embed-text-v2-moe` (475 M total / 305 M active, 768-dim, Matryoshka output) via `fastembed-rs` (Candle backend).

Both are correct, for different jobs.

## 2. Decision

### 2.1 Two-index design

- **Engine-docs index** — prebuilt on Cold Coffee Labs' developer boxes (or CI) with **Nomic Embed Code**. Ships as `assets/rag/engine_docs.vec.sqlite` (pre-sized, read-only, content-hashed). On first run BLD copies it into `.greywater/rag/engine_docs.vec.sqlite` (mutable for symbol rank updates) and opens it. Re-generated at CI release time; content-hash in a sibling `.meta.json` so the runtime can detect drift.
- **User-project index** — built on-demand with `nomic-embed-text-v2-moe` via `fastembed-rs`. Lives at `.greywater/rag/project.vec.sqlite`. Built on first run in the background; updated incrementally on file save.

This split lets us keep "engine context" precisely aligned to the source tree we shipped (and updated only with releases), while the fast-moving user-project content gets a smaller-but-good-enough runtime embedder.

### 2.2 Storage — `sqlite-vec`

Loaded via `rusqlite::Connection::load_extension("sqlite_vec", None)`. Pinned to a specific `sqlite-vec` version in `Cargo.toml` (we ship the compiled extension alongside the executable so users don't need a system install).

Schema (applies to both indices):

```sql
-- Standard rows.
CREATE TABLE IF NOT EXISTS chunks (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    file          TEXT NOT NULL,
    file_hash     TEXT NOT NULL,         -- BLAKE3 of file content at embed time
    span_start    INTEGER NOT NULL,      -- byte offset
    span_end      INTEGER NOT NULL,
    kind          TEXT NOT NULL,         -- "fn" | "class" | "struct" | "doc" | "markdown"
    symbol_rank   REAL NOT NULL DEFAULT 0.0,
    text          TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_chunks_file ON chunks(file);
CREATE INDEX IF NOT EXISTS idx_chunks_kind ON chunks(kind);

-- Vector virtual table (sqlite-vec).
CREATE VIRTUAL TABLE IF NOT EXISTS chunk_vec USING vec0(
    id INTEGER PRIMARY KEY,
    embedding FLOAT[768]
);

-- Symbol graph (for structural retrieval + PageRank).
CREATE TABLE IF NOT EXISTS symbols (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    name          TEXT NOT NULL,
    file          TEXT NOT NULL,
    span_start    INTEGER NOT NULL,
    span_end      INTEGER NOT NULL,
    kind          TEXT NOT NULL,         -- "fn" | "class" | "struct" | "enum" | "macro"
    rank          REAL NOT NULL DEFAULT 0.0
);

CREATE INDEX IF NOT EXISTS idx_symbols_name ON symbols(name);

CREATE TABLE IF NOT EXISTS edges (
    src           INTEGER NOT NULL,
    dst           INTEGER NOT NULL,
    kind          TEXT NOT NULL,         -- "Call" | "Inherit" | "Contain" | "Use"
    PRIMARY KEY (src, dst, kind)
);

-- Embedding model provenance so mixed-model rows cannot slip in.
CREATE TABLE IF NOT EXISTS meta (
    k TEXT PRIMARY KEY,
    v TEXT NOT NULL
);
-- meta: { "model_name", "model_rev", "schema_version", "built_at_utc" }
```

### 2.3 Chunking — tree-sitter

`tree-sitter` grammars we depend on:

- `tree-sitter-cpp`, `tree-sitter-c`, `tree-sitter-rust`, `tree-sitter-glsl`, `tree-sitter-json`, `tree-sitter-md` (markdown).
- **Two custom grammars**, shipped in-tree at `bld/bld-rag/grammars/`:
  - `.gwvs` — the vscript IR grammar (ADR-0009).
  - `.gwscene` — text headers only (the binary body is not tree-sittable; chunker stops at the header).

Chunk boundaries respect function / class / struct / impl / namespace scopes. Mid-function cuts are forbidden; exception: a function body > 512 lines is split by sibling block boundaries, never mid-statement. Markdown chunks use H2/H3 as boundaries; long paragraphs are kept whole up to 1 024 tokens.

### 2.4 Symbol graph & PageRank

Second tree-sitter pass walks all function-call expressions, class-inheritance clauses, struct-containment relations, and type usages. The walk builds a `petgraph::DiGraph<SymbolId, EdgeKind>`. Ranks are computed via `petgraph::algo::page_rank` (α = 0.85, 64 iterations) at index build and after any batch of ≥ 100 symbol edges changes. Symbol rank writes back to `symbols.rank` and the derived `chunks.symbol_rank` (sum of ranks for symbols whose span overlaps the chunk; normalised).

### 2.5 Hybrid retrieval

For query `q`:

- **Lane A (lexical).** `grep`-like scan via `ripgrep` (through `grep` crate) for N_lex = 50 hits. Score = 1.0 for exact case-sensitive match, 0.5 for case-insensitive, 0.3 for substring. De-duped per chunk.
- **Lane B (vector).** Embed `q`; `vec0` nearest-neighbour for N_vec = 50 chunks. Score in `[0, 1]`, cosine similarity.
- **Lane C (symbol graph).** If any exact-name match in `q` exists in `symbols`, walk the 1-hop neighbourhood; score = `dst.rank`; N_sym ≤ 20.

Re-rank combining: `score = 0.4 * vec + 0.3 * lex + 0.3 * sym_rank_norm`. Top-10 returned to the agent.

### 2.6 Watcher — incremental re-index

`notify` + `notify-debouncer-mini` on the project root (and only under `.greywater/config/rag_roots.toml`-listed directories; `target/`, `build/`, `node_modules/` always excluded). Debounce 2 s. Embedding job deferred another 30 s after last edit — the **quiet window**. No embedding runs while typing.

On fire:
1. For each changed file, recompute its BLAKE3 hash.
2. If the hash matches the stored `file_hash`, skip.
3. Else: delete all `chunks` rows with `file = ?`, rechunk, re-embed, insert. Same for `symbols` and `edges` where `file = ?`.
4. If ≥ 100 edges changed, re-run PageRank and update `rank` columns.

### 2.7 Tools introduced by this ADR

- `docs.search(query: string, k?: int, scope?: "engine"|"project"|"all") -> [Hit]`
- `docs.get_file(path: string) -> string`
- `docs.get_symbol(name: string) -> [SymbolLoc]`
- `project.find_refs(name: string) -> [SymbolRef]`
- `project.current_selection() -> EntitySelection` (bridges to editor selection)
- `project.viewport_screenshot() -> BlobRef` (bridges to editor viewport)

All six are **Read-tier** (no mutations; no elicitation).

### 2.8 Determinism

Model names and revisions are written into the `meta` table. On open, BLD compares `meta.model_name` / `meta.model_rev` against the embedder currently wired; a mismatch forces a full re-index (engine-docs index re-copy; project index from scratch). This enforces non-negotiable #16 at RAG scope — the same project at the same commit rebuilt fresh yields bit-identical rows modulo model stochastic jitter (for which we set a seed in `fastembed::InitOptions::with_seed(0)`).

## 3. Alternatives considered

- **`sqlite-vss`.** Deprecated upstream. Rejected.
- **`tantivy` for everything (lexical + vector-ish).** Rejected: tantivy's vector support is less mature; running two backends complicates debugging. sqlite-vec gives us one store.
- **Qdrant as an external service.** Rejected: user-local tools should not require running a Docker container. Also violates the "RX 580 box is the deploy target" spirit.
- **`Swiftide` framework.** Cherry-pick the async streaming ingestion *pattern* (see §2.6); do not adopt the dependency — our chunker needs `tree-sitter-gwvs` and `tree-sitter-gwscene` grammars that are not first-class Swiftide citizens.

## 4. Consequences

- One extension (`sqlite-vec`) shipped alongside the editor executable; one-line `load_extension` at connection open.
- Two-path embedding (cloud-prebuilt large model + runtime small model) is explicit; no single-model compromise.
- PageRank is a batch job; it does not block the hot path.
- Watcher's 30-s quiet window is user-visible; documented in `BUILDING.md`.

## 5. Follow-ups

- `docs/02 §13` amended to reflect `sqlite-vec` + dual-model design.
- `docs/04 §3.5` updated to show the two embedding paths.
- Phase 17 will revisit whether symbol-graph ranks should roll up to cross-file scores.

## 6. References

- `sqlite-vec`: <https://github.com/asg017/sqlite-vec>
- Nomic Embed Text v2 MoE: <https://huggingface.co/nomic-ai/nomic-embed-text-v2-moe>
- `petgraph::algo::page_rank`: <https://docs.rs/petgraph>
- `tree-sitter-graph`: <https://docs.rs/tree-sitter-graph>
