//! `bld-replay` — library half of `gw_agent_replay`.
//!
//! The binary wires CLI parsing; the library owns the policy. Keeping the
//! logic testable in-library is part of the ADR-0015 §2.3 contract: every
//! replay decision (match, mismatch, skip, error) must be exercised by
//! unit tests before the binary ships.
//!
//! What replay does, concretely:
//!
//! 1. Open an audit JSONL produced by `bld-governance::AuditLog`.
//! 2. Stream each `AuditEvent` in `seq` order.
//! 3. For each event, re-request tool dispatch from a pluggable
//!    `ReplayHost` (a trait the CLI wires to either a live editor or a
//!    mock for smoke tests).
//! 4. Compare the actual result digest to the recorded one.
//! 5. Accumulate a `ReplayReport` with per-event verdicts.
//!
//! Determinism: the library never reads wall time, never touches the
//! network, and does not allocate a tokio runtime. If the caller wants
//! concurrency, they wrap the library. This is intentional — replay must
//! be bit-identical across runs.

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

use std::path::Path;

use bld_governance::audit::{canonical_json, read_jsonl, AuditError, AuditEvent};
use serde_json::Value as JsonValue;

/// Replay errors.
#[derive(Debug, thiserror::Error)]
pub enum ReplayError {
    /// Failed to read the audit file.
    #[error("audit: {0}")]
    Audit(#[from] AuditError),
    /// The host call failed for a reason that aborts replay.
    #[error("host: {0}")]
    Host(String),
    /// The audit file is empty.
    #[error("audit file is empty")]
    Empty,
    /// The audit file has a gap in `seq` numbers.
    #[error("audit gap detected: expected seq {expected}, found {actual}")]
    Gap {
        /// Expected next seq.
        expected: u64,
        /// Actual seq read.
        actual: u64,
    },
}

/// Trait a caller implements to re-dispatch tools during replay.
pub trait ReplayHost {
    /// Re-invoke a tool by id with `args`, returning the JSON result.
    ///
    /// The host must be deterministic for replay to be meaningful. A
    /// production host proxies into the editor's tool registry; a test
    /// host returns canned values.
    fn invoke(
        &mut self,
        tool_id: &str,
        args: &JsonValue,
    ) -> Result<JsonValue, String>;
}

/// Per-event verdict.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Verdict {
    /// Result digest matched — byte-identical replay.
    Match,
    /// Result digest diverged — replay drift.
    Drift {
        /// Recorded digest (base64-url no-pad).
        recorded: String,
        /// Actual digest (base64-url no-pad).
        actual: String,
    },
    /// The host refused to run this tool.
    HostError(String),
    /// The event was a non-Ok record we never re-run (budget, denial).
    Skipped,
}

/// One entry in the report.
#[derive(Debug, Clone)]
pub struct VerdictEntry {
    /// Seq copied from the audit record.
    pub seq: u64,
    /// Tool id.
    pub tool_id: String,
    /// The verdict.
    pub verdict: Verdict,
}

/// Aggregate replay report.
#[derive(Debug, Clone, Default)]
pub struct ReplayReport {
    /// All per-event verdicts in seq order.
    pub entries: Vec<VerdictEntry>,
    /// Number of `Match` verdicts.
    pub matches: usize,
    /// Number of `Drift` verdicts.
    pub drifts: usize,
    /// Number of `HostError` verdicts.
    pub host_errors: usize,
    /// Number of `Skipped` verdicts.
    pub skipped: usize,
}

impl ReplayReport {
    /// True if every non-skipped event matched.
    pub fn is_fully_match(&self) -> bool {
        self.drifts == 0 && self.host_errors == 0
    }
}

/// Replay every event in a JSONL file against a `ReplayHost`.
pub fn replay_file<H: ReplayHost>(
    path: &Path,
    archive: Option<&Path>,
    host: &mut H,
) -> Result<ReplayReport, ReplayError> {
    let events = read_jsonl(path)?;
    if events.is_empty() {
        return Err(ReplayError::Empty);
    }
    let archive_payloads = match archive {
        Some(p) => read_archive(p)?,
        None => Vec::new(),
    };
    replay_events(&events, &archive_payloads, host)
}

/// Replay an in-memory list of events. Exposed for testing.
pub fn replay_events<H: ReplayHost>(
    events: &[AuditEvent],
    archive: &[(u64, JsonValue, JsonValue)],
    host: &mut H,
) -> Result<ReplayReport, ReplayError> {
    let mut report = ReplayReport::default();
    let mut expected_seq = events
        .first()
        .map(|e| e.seq)
        .unwrap_or(1);

    for ev in events {
        if ev.seq != expected_seq {
            return Err(ReplayError::Gap {
                expected: expected_seq,
                actual: ev.seq,
            });
        }
        expected_seq = expected_seq.saturating_add(1);

        // Non-Ok outcomes are recorded but never re-executed.
        if !matches!(ev.outcome, bld_governance::audit::ToolOutcome::Ok) {
            report.skipped += 1;
            report.entries.push(VerdictEntry {
                seq: ev.seq,
                tool_id: ev.tool_id.clone(),
                verdict: Verdict::Skipped,
            });
            continue;
        }

        // Pull args/result from the archive (if available). Without the
        // archive we can still verify that the host returns *some*
        // result, just not that the inputs match.
        let args = archive
            .iter()
            .find(|(seq, _, _)| *seq == ev.seq)
            .map(|(_, a, _)| a.clone())
            .unwrap_or_else(|| JsonValue::Null);

        let verdict = match host.invoke(&ev.tool_id, &args) {
            Ok(result) => {
                let bytes = canonical_json(&result);
                let actual = bld_governance::audit::blake3_b64(&bytes);
                if actual == ev.result_digest {
                    report.matches += 1;
                    Verdict::Match
                } else {
                    report.drifts += 1;
                    Verdict::Drift {
                        recorded: ev.result_digest.clone(),
                        actual,
                    }
                }
            }
            Err(e) => {
                report.host_errors += 1;
                Verdict::HostError(e)
            }
        };
        report.entries.push(VerdictEntry {
            seq: ev.seq,
            tool_id: ev.tool_id.clone(),
            verdict,
        });
    }

    Ok(report)
}

/// Read the sibling `.archive.jsonl` file. Returns `(seq, args, result)`.
pub fn read_archive(path: &Path) -> Result<Vec<(u64, JsonValue, JsonValue)>, ReplayError> {
    use std::fs::File;
    use std::io::{BufRead, BufReader};
    if !path.exists() {
        return Ok(Vec::new());
    }
    let f = File::open(path).map_err(|e| ReplayError::Host(e.to_string()))?;
    let reader = BufReader::new(f);
    let mut out = Vec::new();
    for line in reader.lines() {
        let line = line.map_err(|e| ReplayError::Host(e.to_string()))?;
        if line.trim().is_empty() {
            continue;
        }
        let v: JsonValue = serde_json::from_str(&line)
            .map_err(|e| ReplayError::Host(e.to_string()))?;
        let seq = v
            .get("seq")
            .and_then(|s| s.as_u64())
            .ok_or_else(|| ReplayError::Host("archive row missing seq".to_owned()))?;
        let args = v.get("args").cloned().unwrap_or(JsonValue::Null);
        let result = v.get("result").cloned().unwrap_or(JsonValue::Null);
        out.push((seq, args, result));
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;
    use bld_governance::audit::{AuditLog, TierDecision, ToolOutcome};
    use bld_governance::tier::Tier;
    use serde_json::json;
    use std::collections::HashMap;

    /// Test host that returns a fixed map of results.
    struct MapHost {
        results: HashMap<String, JsonValue>,
    }

    impl MapHost {
        fn new() -> Self {
            Self { results: HashMap::new() }
        }
        fn set(&mut self, tool: &str, v: JsonValue) -> &mut Self {
            self.results.insert(tool.to_owned(), v);
            self
        }
    }

    impl ReplayHost for MapHost {
        fn invoke(&mut self, tool_id: &str, _args: &JsonValue) -> Result<JsonValue, String> {
            self.results
                .get(tool_id)
                .cloned()
                .ok_or_else(|| format!("unknown tool {tool_id}"))
        }
    }

    #[test]
    fn empty_file_errors() {
        let mut h = MapHost::new();
        let r = replay_events(&[], &[], &mut h);
        // With no events we return an empty report from replay_events
        // directly (Err only emanates from `replay_file`).
        let report = r.unwrap();
        assert!(report.entries.is_empty());
    }

    #[test]
    fn match_when_result_matches_recorded_digest() {
        let dir = tempfile::tempdir().unwrap();
        let mut log = AuditLog::open(dir.path(), "sess-match").unwrap();
        let result = json!({"handle": 42});
        log.append(
            "turn1",
            "scene.create_entity",
            Tier::Mutate,
            TierDecision::Approved,
            &json!({"name": "Cube"}),
            &result,
            1,
            None,
            ToolOutcome::Ok,
            None,
        )
        .unwrap();
        let evs = log.read_all().unwrap();
        let mut h = MapHost::new();
        h.set("scene.create_entity", result);
        let report = replay_events(&evs, &[], &mut h).unwrap();
        assert_eq!(report.matches, 1);
        assert!(report.is_fully_match());
    }

    #[test]
    fn drift_when_result_differs() {
        let dir = tempfile::tempdir().unwrap();
        let mut log = AuditLog::open(dir.path(), "sess-drift").unwrap();
        log.append(
            "turn1",
            "scene.create_entity",
            Tier::Mutate,
            TierDecision::Approved,
            &json!({"name": "Cube"}),
            &json!({"handle": 42}),
            1,
            None,
            ToolOutcome::Ok,
            None,
        )
        .unwrap();
        let evs = log.read_all().unwrap();
        let mut h = MapHost::new();
        h.set("scene.create_entity", json!({"handle": 99}));
        let report = replay_events(&evs, &[], &mut h).unwrap();
        assert_eq!(report.drifts, 1);
        assert!(!report.is_fully_match());
    }

    #[test]
    fn non_ok_outcomes_are_skipped() {
        let dir = tempfile::tempdir().unwrap();
        let mut log = AuditLog::open(dir.path(), "sess-skip").unwrap();
        log.append(
            "turn1",
            "scene.destroy_entity",
            Tier::Mutate,
            TierDecision::Denied,
            &json!({"e": 1}),
            &json!(null),
            0,
            None,
            ToolOutcome::Denied,
            Some("user rejected".into()),
        )
        .unwrap();
        let evs = log.read_all().unwrap();
        let mut h = MapHost::new();
        let report = replay_events(&evs, &[], &mut h).unwrap();
        assert_eq!(report.skipped, 1);
        assert!(report.is_fully_match());
    }

    #[test]
    fn seq_gap_errors() {
        let mut ev1 = make_event(1);
        ev1.result_digest = bld_governance::audit::blake3_b64(
            &bld_governance::audit::canonical_json(&json!(1)),
        );
        let mut ev3 = make_event(3);
        ev3.result_digest = bld_governance::audit::blake3_b64(
            &bld_governance::audit::canonical_json(&json!(1)),
        );
        let mut h = MapHost::new();
        h.set("t", json!(1));
        let r = replay_events(&[ev1, ev3], &[], &mut h);
        assert!(matches!(r, Err(ReplayError::Gap { expected: 2, actual: 3 })));
    }

    fn make_event(seq: u64) -> AuditEvent {
        AuditEvent {
            seq,
            ts_utc: "1970-01-01T00:00:00.000Z".into(),
            session_id: "s".into(),
            turn_id: "t".into(),
            tool_id: "t".into(),
            tier: Tier::Read,
            args_digest: "".into(),
            args_bytes: 0,
            result_digest: "".into(),
            result_bytes: 0,
            tier_decision: TierDecision::Implicit,
            elapsed_ms: 0,
            provider: None,
            outcome: ToolOutcome::Ok,
            error: None,
        }
    }
}
