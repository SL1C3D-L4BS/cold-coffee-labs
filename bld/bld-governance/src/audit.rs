//! Append-only audit log + session replay primitives (ADR-0015 §2.2 / §2.3).
//!
//! Each tool invocation appends one JSON line to
//! `.greywater/agent/audit/<session_id>/<yyyy-mm-dd>.jsonl`. Digests are
//! BLAKE3 over canonical-JSON (keys sorted, no whitespace). Full payloads
//! are stored in a sibling `.archive.jsonl` only when the per-session
//! archive flag is on.

use std::fs::OpenOptions;
use std::io::{BufRead, BufReader, Write};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};
use serde_json::Value as JsonValue;

use crate::tier::{GovernanceDecision, Tier};

/// Audit errors.
#[derive(Debug, thiserror::Error)]
pub enum AuditError {
    /// I/O failure writing the log.
    #[error("audit io: {0}")]
    Io(#[from] std::io::Error),
    /// JSON serialisation failure (shouldn't happen with our shapes).
    #[error("audit serde: {0}")]
    Serde(#[from] serde_json::Error),
    /// The session dir could not be created.
    #[error("session dir missing: {0}")]
    MissingDir(PathBuf),
    /// Malformed or inconsistent record read.
    #[error("malformed record: {0}")]
    Malformed(&'static str),
}

/// Short outcome enum used in audit records.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "PascalCase")]
pub enum ToolOutcome {
    /// Tool ran and returned Ok.
    Ok,
    /// Tool returned an error (see `error`).
    Err,
    /// Tool call was cancelled (user pressed Stop, or timed out).
    Cancelled,
    /// Secret filter blocked the call before dispatch.
    SecretFilterHit,
    /// The governance decision was Deny.
    Denied,
    /// A budget guardrail tripped (step cap, turn timeout).
    BudgetExceeded,
}

/// Typed governance decision for audit clarity. We re-export the text here
/// to keep the audit record provider-free.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub enum TierDecision {
    /// Approved.
    Approved,
    /// Approved from cache.
    ApprovedCached,
    /// Approved by session grant.
    ApprovedGranted,
    /// Implicit (read-tier).
    Implicit,
    /// Denied.
    Denied,
}

impl From<GovernanceDecision> for TierDecision {
    fn from(d: GovernanceDecision) -> Self {
        match d {
            GovernanceDecision::Approved => Self::Approved,
            GovernanceDecision::ApprovedCached => Self::ApprovedCached,
            GovernanceDecision::ApprovedGranted => Self::ApprovedGranted,
            GovernanceDecision::Implicit => Self::Implicit,
            GovernanceDecision::Denied => Self::Denied,
        }
    }
}

/// One audit record per tool invocation.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AuditEvent {
    /// Sequence number (monotonic per session).
    pub seq: u64,
    /// UTC timestamp, RFC3339 with millisecond precision.
    pub ts_utc: String,
    /// ULID of the session.
    pub session_id: String,
    /// ULID of the turn (one user message -> N tool calls -> 1 response).
    pub turn_id: String,
    /// Tool id.
    pub tool_id: String,
    /// Tier of the tool.
    pub tier: Tier,
    /// BLAKE3 digest of canonical-JSON args (base64-url, no padding).
    pub args_digest: String,
    /// Args size in bytes.
    pub args_bytes: u64,
    /// BLAKE3 digest of canonical-JSON result.
    pub result_digest: String,
    /// Result size in bytes.
    pub result_bytes: u64,
    /// Tier decision path.
    pub tier_decision: TierDecision,
    /// Wall-clock elapsed (ms) for the dispatch.
    pub elapsed_ms: u64,
    /// Provider used for reasoning (if applicable).
    pub provider: Option<String>,
    /// Outcome.
    pub outcome: ToolOutcome,
    /// Optional error message.
    pub error: Option<String>,
}

/// Canonicalise a JSON value for hashing: keys sorted recursively, no
/// whitespace, numbers preserved as written (we delegate to serde_json).
pub fn canonical_json(v: &JsonValue) -> Vec<u8> {
    fn sort(v: JsonValue) -> JsonValue {
        use serde_json::Map;
        match v {
            JsonValue::Object(m) => {
                let mut sorted: Vec<(String, JsonValue)> = m.into_iter().collect();
                sorted.sort_by(|a, b| a.0.cmp(&b.0));
                let mut out = Map::with_capacity(sorted.len());
                for (k, v) in sorted {
                    out.insert(k, sort(v));
                }
                JsonValue::Object(out)
            }
            JsonValue::Array(a) => {
                JsonValue::Array(a.into_iter().map(sort).collect())
            }
            other => other,
        }
    }
    let sorted = sort(v.clone());
    // Safe because we just built `sorted` from `v`.
    serde_json::to_vec(&sorted).unwrap_or_default()
}

/// Compute BLAKE3 digest, base64-url-no-pad.
pub fn blake3_b64(bytes: &[u8]) -> String {
    let digest = blake3::hash(bytes);
    let mut s = String::with_capacity(43);
    encode_b64_url(digest.as_bytes(), &mut s);
    s
}

fn encode_b64_url(input: &[u8], out: &mut String) {
    const ALPH: &[u8; 64] =
        b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    let mut buf = [0u8; 4];
    for chunk in input.chunks(3) {
        let b0 = chunk.first().copied().unwrap_or(0);
        let b1 = chunk.get(1).copied().unwrap_or(0);
        let b2 = chunk.get(2).copied().unwrap_or(0);
        buf[0] = ALPH[(b0 >> 2) as usize];
        buf[1] = ALPH[(((b0 & 0x03) << 4) | (b1 >> 4)) as usize];
        buf[2] = ALPH[(((b1 & 0x0f) << 2) | (b2 >> 6)) as usize];
        buf[3] = ALPH[(b2 & 0x3f) as usize];
        let n = match chunk.len() {
            1 => 2,
            2 => 3,
            _ => 4,
        };
        for b in &buf[..n] {
            out.push(*b as char);
        }
    }
}

/// The append-only log writer.
pub struct AuditLog {
    /// UTC date key for the current file (e.g. `"2026-04-24"`).
    date: String,
    dir: PathBuf,
    seq: u64,
    session_id: String,
    archive_full_payloads: bool,
}

impl AuditLog {
    /// Open (or create) an audit log under the given root.
    ///
    /// Layout: `<root>/.greywater/agent/audit/<session_id>/<yyyy-mm-dd>.jsonl`.
    pub fn open(root: &Path, session_id: &str) -> Result<Self, AuditError> {
        let today = current_date_utc();
        let dir = root
            .join(".greywater")
            .join("agent")
            .join("audit")
            .join(session_id);
        std::fs::create_dir_all(&dir)?;
        // Determine last seq by scanning existing file, if any.
        let file = dir.join(format!("{today}.jsonl"));
        let seq = last_seq(&file)?;
        Ok(Self {
            date: today,
            dir,
            seq,
            session_id: session_id.to_owned(),
            archive_full_payloads: false,
        })
    }

    /// Enable archiving full payloads (off by default; ADR-0015 privacy).
    pub fn with_archive(mut self, on: bool) -> Self {
        self.archive_full_payloads = on;
        self
    }

    /// Append one record.
    ///
    /// `args` and `result` are the raw JSON values; the record stores
    /// digests + sizes. If archiving is on, full payloads go to
    /// `<date>.archive.jsonl` as a sibling file.
    #[allow(clippy::too_many_arguments)]
    pub fn append(
        &mut self,
        turn_id: &str,
        tool_id: &str,
        tier: Tier,
        tier_decision: TierDecision,
        args: &JsonValue,
        result: &JsonValue,
        elapsed_ms: u64,
        provider: Option<&str>,
        outcome: ToolOutcome,
        error: Option<String>,
    ) -> Result<AuditEvent, AuditError> {
        let args_bytes = canonical_json(args);
        let result_bytes = canonical_json(result);
        let ev = AuditEvent {
            seq: self.next_seq(),
            ts_utc: current_ts_utc(),
            session_id: self.session_id.clone(),
            turn_id: turn_id.to_owned(),
            tool_id: tool_id.to_owned(),
            tier,
            args_digest: blake3_b64(&args_bytes),
            args_bytes: args_bytes.len() as u64,
            result_digest: blake3_b64(&result_bytes),
            result_bytes: result_bytes.len() as u64,
            tier_decision,
            elapsed_ms,
            provider: provider.map(str::to_owned),
            outcome,
            error,
        };
        // Rotate if the date has flipped since the log was opened.
        let today = current_date_utc();
        if today != self.date {
            self.date = today;
        }
        let path = self.dir.join(format!("{}.jsonl", self.date));
        let mut f = OpenOptions::new().create(true).append(true).open(&path)?;
        let line = serde_json::to_string(&ev)?;
        f.write_all(line.as_bytes())?;
        f.write_all(b"\n")?;
        if self.archive_full_payloads {
            let ap = self.dir.join(format!("{}.archive.jsonl", self.date));
            let mut af = OpenOptions::new().create(true).append(true).open(&ap)?;
            let rec = serde_json::json!({
                "seq": ev.seq,
                "args": args,
                "result": result,
            });
            let line = serde_json::to_string(&rec)?;
            af.write_all(line.as_bytes())?;
            af.write_all(b"\n")?;
        }
        Ok(ev)
    }

    /// Current next seq value.
    fn next_seq(&mut self) -> u64 {
        self.seq += 1;
        self.seq
    }

    /// Iterate every record written to the current file, in order. Used by
    /// `gw_agent_replay`.
    pub fn read_all(&self) -> Result<Vec<AuditEvent>, AuditError> {
        let path = self.dir.join(format!("{}.jsonl", self.date));
        read_jsonl(&path)
    }

    /// Dir of the session (for external readers).
    pub fn dir(&self) -> &Path {
        &self.dir
    }
}

fn last_seq(path: &Path) -> Result<u64, AuditError> {
    if !path.exists() {
        return Ok(0);
    }
    let f = std::fs::File::open(path)?;
    let reader = BufReader::new(f);
    let mut last = 0u64;
    for line in reader.lines() {
        let line = line?;
        if line.trim().is_empty() {
            continue;
        }
        let ev: AuditEvent = serde_json::from_str(&line)?;
        if ev.seq > last {
            last = ev.seq;
        }
    }
    Ok(last)
}

/// Read every record in a JSONL file.
pub fn read_jsonl(path: &Path) -> Result<Vec<AuditEvent>, AuditError> {
    if !path.exists() {
        return Ok(Vec::new());
    }
    let f = std::fs::File::open(path)?;
    let reader = BufReader::new(f);
    let mut out = Vec::new();
    for line in reader.lines() {
        let line = line?;
        if line.trim().is_empty() {
            continue;
        }
        let ev: AuditEvent = serde_json::from_str(&line)?;
        out.push(ev);
    }
    Ok(out)
}

/// Current UTC date in `yyyy-mm-dd`.
fn current_date_utc() -> String {
    let d = time_days_since_epoch_utc();
    format_ymd(d)
}

/// Current UTC timestamp in RFC3339 (ms).
fn current_ts_utc() -> String {
    // Minimal RFC3339 without pulling in chrono. We produce
    // yyyy-mm-ddTHH:MM:SS.sssZ. Good enough for audit logs.
    let now = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default();
    let secs = now.as_secs();
    let ms = now.subsec_millis();
    let (h, m, s) = {
        let day_secs = (secs % 86_400) as u32;
        let h = day_secs / 3600;
        let m = (day_secs % 3600) / 60;
        let s = day_secs % 60;
        (h, m, s)
    };
    let days = (secs / 86_400) as i64;
    let (y, mo, d) = days_to_ymd(days);
    format!("{y:04}-{mo:02}-{d:02}T{h:02}:{m:02}:{s:02}.{ms:03}Z")
}

fn time_days_since_epoch_utc() -> i64 {
    let now = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default();
    (now.as_secs() / 86_400) as i64
}

fn format_ymd(days: i64) -> String {
    let (y, m, d) = days_to_ymd(days);
    format!("{y:04}-{m:02}-{d:02}")
}

/// Convert days-since-1970-01-01 to (y, m, d). Valid 1970..2200.
fn days_to_ymd(mut days: i64) -> (i32, u32, u32) {
    let mut year = 1970i32;
    loop {
        let leap = is_leap(year);
        let year_days = if leap { 366 } else { 365 };
        if days < year_days as i64 {
            break;
        }
        days -= year_days as i64;
        year += 1;
    }
    let leap = is_leap(year);
    let months_len: [u32; 12] = [
        31,
        if leap { 29 } else { 28 },
        31,
        30,
        31,
        30,
        31,
        31,
        30,
        31,
        30,
        31,
    ];
    let mut m = 1u32;
    for len in months_len {
        if days < len as i64 {
            break;
        }
        days -= len as i64;
        m += 1;
    }
    (year, m, (days as u32) + 1)
}

fn is_leap(y: i32) -> bool {
    (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;
    use tempfile::tempdir;

    #[test]
    fn b64_basic() {
        let out = blake3_b64(b"hello");
        // blake3 is deterministic; test format only (length + charset).
        assert_eq!(out.len(), 43);
        assert!(out.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_'));
    }

    #[test]
    fn canonical_json_sorts_keys() {
        let a = canonical_json(&json!({"b": 1, "a": 2}));
        let b = canonical_json(&json!({"a": 2, "b": 1}));
        assert_eq!(a, b);
    }

    #[test]
    fn canonical_json_recurses() {
        let a = canonical_json(&json!({"x": {"b": 1, "a": 2}, "y": [{"z": 1, "y": 0}]}));
        let b = canonical_json(&json!({"y": [{"y": 0, "z": 1}], "x": {"a": 2, "b": 1}}));
        assert_eq!(a, b);
    }

    #[test]
    fn audit_log_roundtrip() {
        let dir = tempdir().unwrap();
        let mut log = AuditLog::open(dir.path(), "01HW3J9ZC9XN3NGTSTXQG0VZ7K").unwrap();
        let ev = log
            .append(
                "01HW3JA7R5P8Y6TVBDEJRC6X2K",
                "scene.create_entity",
                Tier::Mutate,
                TierDecision::Approved,
                &json!({"name": "Cube"}),
                &json!({"handle": 42}),
                13,
                Some("anthropic:claude-opus-4-7"),
                ToolOutcome::Ok,
                None,
            )
            .unwrap();
        assert_eq!(ev.seq, 1);
        let all = log.read_all().unwrap();
        assert_eq!(all.len(), 1);
        assert_eq!(all[0].tool_id, "scene.create_entity");
        assert_eq!(all[0].outcome, ToolOutcome::Ok);
        // Digests not empty.
        assert!(!all[0].args_digest.is_empty());
    }

    #[test]
    fn audit_log_is_append_only_and_seq_monotonic() {
        let dir = tempdir().unwrap();
        let mut log = AuditLog::open(dir.path(), "sess").unwrap();
        for i in 0..10 {
            log.append(
                "turn1",
                "project.list_scenes",
                Tier::Read,
                TierDecision::Implicit,
                &json!({"i": i}),
                &json!([]),
                1,
                None,
                ToolOutcome::Ok,
                None,
            )
            .unwrap();
        }
        let all = log.read_all().unwrap();
        assert_eq!(all.len(), 10);
        let seqs: Vec<u64> = all.iter().map(|e| e.seq).collect();
        assert_eq!(seqs, (1..=10).collect::<Vec<u64>>());
    }

    #[test]
    fn reopen_resumes_seq() {
        let dir = tempdir().unwrap();
        {
            let mut log = AuditLog::open(dir.path(), "resume").unwrap();
            log.append(
                "t",
                "project.list_scenes",
                Tier::Read,
                TierDecision::Implicit,
                &json!({}),
                &json!([]),
                0,
                None,
                ToolOutcome::Ok,
                None,
            )
            .unwrap();
        }
        let mut log2 = AuditLog::open(dir.path(), "resume").unwrap();
        let ev = log2
            .append(
                "t2",
                "project.list_scenes",
                Tier::Read,
                TierDecision::Implicit,
                &json!({}),
                &json!([]),
                0,
                None,
                ToolOutcome::Ok,
                None,
            )
            .unwrap();
        assert_eq!(ev.seq, 2);
    }

    #[test]
    fn ymd_conversion_roundtrips() {
        // Spot-check four dates that cross leap boundaries. The
        // days-since-1970 figures were verified against GNU date.
        assert_eq!(days_to_ymd(0), (1970, 1, 1));
        assert_eq!(days_to_ymd(59), (1970, 3, 1));          // 31 + 28
        assert_eq!(days_to_ymd(365), (1971, 1, 1));
        assert_eq!(days_to_ymd(731), (1972, 1, 2));         // leap year start check
        assert_eq!(days_to_ymd(20564), (2026, 4, 21));
    }
}
