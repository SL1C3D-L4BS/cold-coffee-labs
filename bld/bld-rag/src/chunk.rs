//! Source chunker.
//!
//! The heuristic chunker splits on blank-line boundaries for prose /
//! markdown and on top-level brace-balanced regions for source code. It
//! is intentionally syntax-unaware so corpora with mixed or unrecognised
//! languages still produce useful chunks; `ts-chunk` feature adds
//! tree-sitter-based chunking for Rust/C++/Markdown where accuracy
//! matters.

use serde::{Deserialize, Serialize};

/// What kind of content a chunk contains (for ranking + filtering).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ChunkKind {
    /// Prose / markdown / docs.
    Prose,
    /// Function or method body.
    Function,
    /// Type declaration (struct, enum, class).
    TypeDecl,
    /// Top-level statements (imports, consts).
    Toplevel,
    /// Comment run.
    Comment,
    /// Unknown — fall-back.
    Other,
}

/// A single chunk of source or docs.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Chunk {
    /// Source path (relative, forward-slashed).
    pub path: String,
    /// First line (1-based, inclusive).
    pub start_line: u32,
    /// Last line (1-based, inclusive).
    pub end_line: u32,
    /// The chunk body.
    pub body: String,
    /// What kind of region this is.
    pub kind: ChunkKind,
    /// BLAKE3 hex digest of `body`.
    pub digest: String,
}

impl Chunk {
    /// Compute a BLAKE3 hex digest for a body.
    #[must_use]
    pub fn digest_of(body: &str) -> String {
        blake3::hash(body.as_bytes()).to_hex().to_string()
    }
}

/// Configurable heuristic chunker.
#[derive(Debug, Clone)]
pub struct Chunker {
    /// Soft target for chunk size in bytes; the chunker splits at natural
    /// boundaries close to this value.
    pub target_bytes: usize,
    /// Hard max; no chunk exceeds this.
    pub max_bytes: usize,
    /// Hard min; adjacent short chunks merge until >= this (unless EOF).
    pub min_bytes: usize,
}

impl Default for Chunker {
    fn default() -> Self {
        Self {
            target_bytes: 1024,
            max_bytes: 4096,
            min_bytes: 128,
        }
    }
}

impl Chunker {
    /// Create a chunker with an explicit target.
    pub fn new(target_bytes: usize, max_bytes: usize, min_bytes: usize) -> Self {
        // min_bytes is the post-merge floor and may exceed target when the
        // caller wants small fragments glued together aggressively.
        assert!(target_bytes <= max_bytes, "target must be <= max");
        Self {
            target_bytes,
            max_bytes,
            min_bytes,
        }
    }

    /// Chunk prose (markdown / plain) by blank-line paragraph groups.
    pub fn chunk_prose(&self, path: &str, text: &str) -> Vec<Chunk> {
        let mut out = Vec::new();
        let mut line = 1_u32;
        let mut start_line = 1_u32;
        let mut buf = String::new();
        let mut buf_start = 1_u32;

        for para in split_paragraphs(text) {
            // Count lines in this paragraph.
            let para_lines = para.matches('\n').count() as u32 + 1;
            if buf.is_empty() {
                buf_start = start_line;
            }
            if !buf.is_empty() && buf.len() + para.len() + 2 > self.target_bytes {
                out.push(Chunk {
                    path: path.to_owned(),
                    start_line: buf_start,
                    end_line: line - 1,
                    body: std::mem::take(&mut buf),
                    kind: ChunkKind::Prose,
                    digest: String::new(),
                });
                buf_start = line;
            }
            if !buf.is_empty() {
                buf.push_str("\n\n");
            }
            buf.push_str(para);
            line += para_lines + 1;
            start_line = line;
        }

        if !buf.is_empty() {
            out.push(Chunk {
                path: path.to_owned(),
                start_line: buf_start,
                end_line: line.saturating_sub(1).max(buf_start),
                body: buf,
                kind: ChunkKind::Prose,
                digest: String::new(),
            });
        }

        // Merge undersized neighbours.
        merge_tiny(&mut out, self.min_bytes);
        // Compute digests.
        for c in &mut out {
            c.digest = Chunk::digest_of(&c.body);
        }
        out
    }

    /// Chunk source code by top-level brace-balanced regions. The
    /// heuristic treats `\n\n` between declarations as a strong split
    /// hint, but never splits inside `{}` / `()` / `[]`.
    pub fn chunk_source(&self, path: &str, text: &str) -> Vec<Chunk> {
        let mut out = Vec::new();
        let lines: Vec<&str> = text.split_inclusive('\n').collect();
        let mut balance: i32 = 0;
        let mut in_str = false;
        let mut in_char = false;
        let mut buf = String::new();
        let mut buf_start = 1_u32;
        let mut cur_line = 1_u32;

        for (idx, raw) in lines.iter().enumerate() {
            if buf.is_empty() {
                buf_start = cur_line;
            }
            let chars: Vec<char> = raw.chars().collect();
            let mut i = 0;
            while i < chars.len() {
                let c = chars[i];
                let next = chars.get(i + 1).copied();
                if !in_str && !in_char {
                    if c == '/' && next == Some('/') {
                        // Line comment, skip rest.
                        break;
                    }
                    match c {
                        '{' | '(' | '[' => balance += 1,
                        '}' | ')' | ']' => balance -= 1,
                        '"' => in_str = true,
                        '\'' => in_char = true,
                        _ => {}
                    }
                } else if in_str {
                    if c == '\\' {
                        i += 1;
                    } else if c == '"' {
                        in_str = false;
                    }
                } else if in_char {
                    if c == '\\' {
                        i += 1;
                    } else if c == '\'' {
                        in_char = false;
                    }
                }
                i += 1;
            }
            buf.push_str(raw);
            cur_line += 1;

            // Decide split: balance back to zero AND we've met the target, OR
            // we've run past max_bytes.
            let at_blank_between = raw.trim().is_empty()
                && lines
                    .get(idx + 1)
                    .is_some_and(|l| !l.trim().is_empty());
            let long_enough = buf.len() >= self.target_bytes;
            let too_long = buf.len() >= self.max_bytes;
            if (balance <= 0 && long_enough && at_blank_between) || too_long {
                out.push(Chunk {
                    path: path.to_owned(),
                    start_line: buf_start,
                    end_line: cur_line - 1,
                    body: std::mem::take(&mut buf),
                    kind: ChunkKind::Toplevel,
                    digest: String::new(),
                });
                balance = 0;
                in_str = false;
                in_char = false;
            }
        }
        if !buf.is_empty() {
            out.push(Chunk {
                path: path.to_owned(),
                start_line: buf_start,
                end_line: cur_line.saturating_sub(1).max(buf_start),
                body: buf,
                kind: ChunkKind::Toplevel,
                digest: String::new(),
            });
        }
        merge_tiny(&mut out, self.min_bytes);
        for c in &mut out {
            c.digest = Chunk::digest_of(&c.body);
            c.kind = classify(&c.body);
        }
        out
    }
}

fn split_paragraphs(text: &str) -> Vec<&str> {
    text.split("\n\n").filter(|s| !s.trim().is_empty()).collect()
}

fn merge_tiny(out: &mut Vec<Chunk>, min_bytes: usize) {
    if out.len() < 2 {
        return;
    }
    let mut i = 0;
    while i + 1 < out.len() {
        if out[i].body.len() < min_bytes && out[i].path == out[i + 1].path {
            let next = out.remove(i + 1);
            out[i].body.push_str(&next.body);
            out[i].end_line = next.end_line;
        } else {
            i += 1;
        }
    }
}

fn classify(body: &str) -> ChunkKind {
    let t = body.trim_start();
    if t.starts_with("fn ")
        || t.starts_with("pub fn")
        || t.starts_with("async fn")
        || t.contains("(")
            && t.contains(")")
            && t.contains("{")
            && t.split_whitespace().next().map_or(false, |w| {
                matches!(w, "void" | "int" | "auto" | "bool" | "float" | "double")
            })
    {
        ChunkKind::Function
    } else if t.starts_with("struct")
        || t.starts_with("pub struct")
        || t.starts_with("enum")
        || t.starts_with("pub enum")
        || t.starts_with("class")
        || t.starts_with("interface")
    {
        ChunkKind::TypeDecl
    } else if t.starts_with("//")
        || t.starts_with("/*")
        || t.starts_with("#")
        || t.starts_with("*")
    {
        ChunkKind::Comment
    } else {
        ChunkKind::Toplevel
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn prose_chunks_split_on_paragraphs() {
        let ch = Chunker::new(64, 256, 1);
        let body = "hello world\n\nthis is the second paragraph with more text.\n\n\
                    third paragraph here.";
        let out = ch.chunk_prose("a.md", body);
        assert!(!out.is_empty());
        for c in &out {
            assert!(!c.body.is_empty());
            assert!(c.body.len() <= 256);
            assert_eq!(c.kind, ChunkKind::Prose);
            assert!(!c.digest.is_empty());
        }
    }

    #[test]
    fn source_chunks_respect_brace_balance() {
        let ch = Chunker::new(128, 512, 1);
        let src = r#"
fn alpha() {
    let a = 1;
    let b = 2;
}

fn beta() {
    "string with } brace";
    let c = 3;
}
"#;
        let out = ch.chunk_source("a.rs", src);
        assert!(!out.is_empty());
        let all: String = out.iter().map(|c| c.body.clone()).collect();
        // Round-trip: content preserved.
        assert_eq!(all.trim(), src.trim());
        // At least one function-kind chunk.
        assert!(out.iter().any(|c| c.kind == ChunkKind::Function));
    }

    #[test]
    fn digests_are_stable_across_calls() {
        let ch = Chunker::default();
        let text = "one paragraph.\n\nanother paragraph.";
        let a = ch.chunk_prose("x.md", text);
        let b = ch.chunk_prose("x.md", text);
        let da: Vec<_> = a.iter().map(|c| c.digest.clone()).collect();
        let db: Vec<_> = b.iter().map(|c| c.digest.clone()).collect();
        assert_eq!(da, db);
    }

    #[test]
    fn tiny_chunks_merge() {
        // target=1 forces each paragraph into its own chunk; min=256 then
        // causes merge_tiny to glue them all back together into one.
        let ch = Chunker::new(1, 512, 256);
        let src = "a\n\nb\n\nc\n\nd\n\ne";
        let out = ch.chunk_prose("t.md", src);
        assert_eq!(out.len(), 1);
    }
}
