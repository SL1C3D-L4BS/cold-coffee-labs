//! BM25 lexical ranker over the chunk index.
//!
//! A tiny, self-contained BM25 implementation so we don't need
//! `tantivy` just for ranking. The ranker tokenises bodies on ASCII
//! whitespace + punctuation, lower-cases, and stores IDF + term
//! frequency tables keyed by chunk index.
//!
//! BM25 parameters default to k1=1.2, b=0.75 — standard values that work
//! well for code and prose alike. `RetrievalConfig` can override.

use std::collections::HashMap;

use crate::corpus::Corpus;

const DEFAULT_K1: f32 = 1.2;
const DEFAULT_B: f32 = 0.75;

/// Pre-computed BM25 index over a corpus snapshot.
#[derive(Debug, Clone)]
pub struct Bm25Index {
    /// Per-chunk term-frequency table.
    pub tf: Vec<HashMap<String, u32>>,
    /// Document frequency per term (across chunks).
    pub df: HashMap<String, u32>,
    /// Length of each chunk in tokens.
    pub chunk_lengths: Vec<u32>,
    /// Average chunk length.
    pub avg_len: f32,
    /// BM25 k1.
    pub k1: f32,
    /// BM25 b.
    pub b: f32,
}

impl Default for Bm25Index {
    fn default() -> Self {
        Self {
            tf: Vec::new(),
            df: HashMap::new(),
            chunk_lengths: Vec::new(),
            avg_len: 0.0,
            k1: DEFAULT_K1,
            b: DEFAULT_B,
        }
    }
}

impl Bm25Index {
    /// Build the index over the current corpus.
    pub fn build(corpus: &Corpus) -> Self {
        let mut tf: Vec<HashMap<String, u32>> = Vec::with_capacity(corpus.chunk_count());
        let mut df: HashMap<String, u32> = HashMap::new();
        let mut lens: Vec<u32> = Vec::with_capacity(corpus.chunk_count());

        for (_, chunk) in corpus.chunks() {
            let tokens = tokenise(&chunk.body);
            let mut seen: HashMap<String, u32> = HashMap::new();
            for t in &tokens {
                *seen.entry(t.clone()).or_insert(0) += 1;
            }
            for t in seen.keys() {
                *df.entry(t.clone()).or_insert(0) += 1;
            }
            lens.push(tokens.len() as u32);
            tf.push(seen);
        }

        let total: u64 = lens.iter().map(|&n| n as u64).sum();
        let avg_len = if lens.is_empty() {
            0.0
        } else {
            total as f32 / lens.len() as f32
        };

        Self {
            tf,
            df,
            chunk_lengths: lens,
            avg_len,
            k1: DEFAULT_K1,
            b: DEFAULT_B,
        }
    }

    /// Score every chunk against a query. Returns `(chunk_index, score)`
    /// pairs with score > 0, not sorted.
    pub fn score_all(&self, query: &str) -> Vec<(usize, f32)> {
        if self.chunk_lengths.is_empty() {
            return Vec::new();
        }
        let n = self.chunk_lengths.len() as f32;
        let q_tokens = tokenise(query);
        let mut out: Vec<(usize, f32)> = Vec::new();
        for (idx, tf) in self.tf.iter().enumerate() {
            let dl = self.chunk_lengths[idx] as f32;
            if dl == 0.0 {
                continue;
            }
            let mut score = 0.0_f32;
            for term in &q_tokens {
                let f = *tf.get(term).unwrap_or(&0) as f32;
                if f == 0.0 {
                    continue;
                }
                let df = *self.df.get(term).unwrap_or(&0) as f32;
                let idf = ((n - df + 0.5) / (df + 0.5) + 1.0).ln();
                let norm = 1.0 - self.b + self.b * (dl / self.avg_len.max(1.0));
                score += idf * ((f * (self.k1 + 1.0)) / (f + self.k1 * norm));
            }
            if score > 0.0 {
                out.push((idx, score));
            }
        }
        out
    }

    /// Score and return top-k, sorted by score descending.
    pub fn top_k(&self, query: &str, k: usize) -> Vec<(usize, f32)> {
        let mut all = self.score_all(query);
        all.sort_by(|a, b| b.1.partial_cmp(&a.1).unwrap_or(std::cmp::Ordering::Equal));
        all.truncate(k);
        all
    }
}

/// Simple ASCII tokeniser: keep alphanumerics + `_`, lower-case, split on
/// everything else. Good enough for code and English prose.
pub fn tokenise(text: &str) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    let mut cur = String::new();
    for c in text.chars() {
        if c.is_ascii_alphanumeric() || c == '_' {
            for lc in c.to_lowercase() {
                cur.push(lc);
            }
        } else if !cur.is_empty() {
            out.push(std::mem::take(&mut cur));
        }
    }
    if !cur.is_empty() {
        out.push(cur);
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::corpus::Corpus;

    #[test]
    fn tokenise_splits_on_punctuation() {
        let t = tokenise("Hello, world! foo_bar - baz.");
        assert_eq!(t, vec!["hello", "world", "foo_bar", "baz"]);
    }

    #[test]
    fn bm25_ranks_matching_chunks_higher() {
        let mut c = Corpus::new();
        c.upsert("a.md", "the quick brown fox jumps over the lazy dog");
        c.upsert("b.md", "rust programming language ownership borrow checker");
        c.upsert("c.md", "dog food and cat food are not the same thing");
        let ix = Bm25Index::build(&c);
        let top = ix.top_k("ownership", 5);
        assert!(!top.is_empty());
        // The Rust chunk should be the top match.
        let best = top[0].0;
        let chunk = c.chunk(best).unwrap();
        assert_eq!(chunk.path, "b.md");
    }

    #[test]
    fn bm25_empty_corpus_returns_empty() {
        let c = Corpus::new();
        let ix = Bm25Index::build(&c);
        assert!(ix.top_k("anything", 10).is_empty());
    }

    #[test]
    fn bm25_query_with_no_matches_is_empty() {
        let mut c = Corpus::new();
        c.upsert("a.md", "the quick brown fox");
        let ix = Bm25Index::build(&c);
        assert!(ix.top_k("zylophones", 10).is_empty());
    }
}
