//! `gw_agent_replay` CLI (polished in Wave 9E).
//!
//! Replays a `bld-governance::AuditLog` against a pluggable
//! [`bld_replay::ReplayHost`] and reports per-event verdicts.
//!
//! ```text
//! gw_agent_replay <audit.jsonl>
//!     [--archive <archive.jsonl>]   include args/result archive
//!     [--strict]                    exit 1 on any drift/host-error
//!     [--bit-identical]             exit 1 if any event is non-Match (stricter than --strict)
//!     [--json]                      emit the full report as JSON to stdout
//!     [--limit N]                   only replay first N events
//!     [--filter <prefix>]           only replay events whose tool_id starts with <prefix>
//!     [-q | --quiet]                suppress per-event lines (summary only)
//! ```
//!
//! Exit codes:
//! * 0 — all events matched (or `--strict` not set and only drift);
//! * 1 — `--strict` and drift, or `--bit-identical` and non-Match;
//! * 2 — usage or I/O error.

use std::path::PathBuf;
use std::process::ExitCode;

use anyhow::{anyhow, Context, Result};
use serde::Serialize;
use serde_json::Value as JsonValue;

use bld_replay::{replay_file, ReplayHost, ReplayReport, Verdict};

/// Trivial CLI host — reports "offline" for every call. In production,
/// the CLI boots the engine in headless mode and delegates to Surface P;
/// this ships as the default so the binary is always usable for offline
/// audit inspection.
struct OfflineHost;

impl ReplayHost for OfflineHost {
    fn invoke(&mut self, _tool_id: &str, _args: &JsonValue) -> Result<JsonValue, String> {
        Err("offline host — replay requires engine attachment".to_owned())
    }
}

#[derive(Debug, Default)]
struct Args {
    audit: Option<PathBuf>,
    archive: Option<PathBuf>,
    strict: bool,
    bit_identical: bool,
    json: bool,
    quiet: bool,
    limit: Option<usize>,
    filter: Option<String>,
}

fn main() -> ExitCode {
    match try_main() {
        Ok(n) => ExitCode::from(n),
        Err(e) => {
            eprintln!("gw_agent_replay: {e:#}");
            ExitCode::from(2)
        }
    }
}

fn try_main() -> Result<u8> {
    let args = parse_args()?;

    let audit = args
        .audit
        .as_ref()
        .ok_or_else(|| anyhow!("missing audit file argument"))?;

    let mut host = OfflineHost;
    let mut report = replay_file(audit, args.archive.as_deref(), &mut host)
        .with_context(|| format!("replaying {}", audit.display()))?;

    if let Some(prefix) = &args.filter {
        report = filter_report(report, prefix);
    }
    if let Some(n) = args.limit {
        report = truncate_report(report, n);
    }

    if args.json {
        print_json(&report);
    } else if !args.quiet {
        print_report(&report);
    } else {
        print_summary(&report);
    }

    if args.bit_identical && !is_bit_identical(&report) {
        return Ok(1);
    }
    if args.strict && !report.is_fully_match() {
        return Ok(1);
    }
    Ok(0)
}

fn parse_args() -> Result<Args> {
    let mut out = Args::default();
    let mut it = std::env::args().skip(1);
    while let Some(a) = it.next() {
        match a.as_str() {
            "--archive" => {
                out.archive = Some(PathBuf::from(
                    it.next().ok_or_else(|| anyhow!("--archive needs a path"))?,
                ));
            }
            "--strict" => out.strict = true,
            "--bit-identical" => out.bit_identical = true,
            "--json" => out.json = true,
            "-q" | "--quiet" => out.quiet = true,
            "--limit" => {
                let v = it.next().ok_or_else(|| anyhow!("--limit needs a number"))?;
                out.limit = Some(v.parse().context("parsing --limit")?);
            }
            "--filter" => {
                out.filter = Some(it.next().ok_or_else(|| anyhow!("--filter needs a prefix"))?);
            }
            "-h" | "--help" => {
                print_help();
                std::process::exit(0);
            }
            other if !other.starts_with("--") && out.audit.is_none() => {
                out.audit = Some(PathBuf::from(other));
            }
            other => return Err(anyhow!("unknown argument: {other}")),
        }
    }
    Ok(out)
}

fn print_help() {
    println!(
        "usage: gw_agent_replay <audit.jsonl> [options]\n\n\
         options:\n  \
         --archive <p>      include args/result archive\n  \
         --strict           exit 1 on drift or host-error\n  \
         --bit-identical    exit 1 on any non-Match event (implies strict)\n  \
         --json             emit the report as JSON\n  \
         --limit N          replay only first N events\n  \
         --filter <prefix>  only events whose tool_id starts with <prefix>\n  \
         -q, --quiet        summary only"
    );
}

fn filter_report(mut r: ReplayReport, prefix: &str) -> ReplayReport {
    r.entries.retain(|e| e.tool_id.starts_with(prefix));
    recount(&mut r);
    r
}

fn truncate_report(mut r: ReplayReport, n: usize) -> ReplayReport {
    r.entries.truncate(n);
    recount(&mut r);
    r
}

fn recount(r: &mut ReplayReport) {
    r.matches = 0;
    r.drifts = 0;
    r.host_errors = 0;
    r.skipped = 0;
    for e in &r.entries {
        match &e.verdict {
            Verdict::Match => r.matches += 1,
            Verdict::Drift { .. } => r.drifts += 1,
            Verdict::HostError(_) => r.host_errors += 1,
            Verdict::Skipped => r.skipped += 1,
        }
    }
}

fn is_bit_identical(r: &ReplayReport) -> bool {
    r.drifts == 0 && r.host_errors == 0 && r.skipped == 0
}

fn print_report(r: &ReplayReport) {
    print_summary(r);
    for e in &r.entries {
        match &e.verdict {
            Verdict::Match => println!("[{:>6}] {:<40} MATCH", e.seq, e.tool_id),
            Verdict::Drift { recorded, actual } => println!(
                "[{:>6}] {:<40} DRIFT recorded={} actual={}",
                e.seq, e.tool_id, recorded, actual
            ),
            Verdict::HostError(msg) => {
                println!("[{:>6}] {:<40} HOST-ERR {msg}", e.seq, e.tool_id)
            }
            Verdict::Skipped => println!("[{:>6}] {:<40} skip", e.seq, e.tool_id),
        }
    }
}

fn print_summary(r: &ReplayReport) {
    println!(
        "replay report: {} events ({} match, {} drift, {} host-error, {} skipped)",
        r.entries.len(),
        r.matches,
        r.drifts,
        r.host_errors,
        r.skipped
    );
}

#[derive(Serialize)]
struct JsonEntry<'a> {
    seq: u64,
    tool_id: &'a str,
    verdict: JsonVerdict<'a>,
}

#[derive(Serialize)]
#[serde(tag = "kind", rename_all = "snake_case")]
enum JsonVerdict<'a> {
    Match,
    Drift { recorded: &'a str, actual: &'a str },
    HostError { message: &'a str },
    Skipped,
}

#[derive(Serialize)]
struct JsonReport<'a> {
    total: usize,
    matches: usize,
    drifts: usize,
    host_errors: usize,
    skipped: usize,
    fully_match: bool,
    bit_identical: bool,
    entries: Vec<JsonEntry<'a>>,
}

fn print_json(r: &ReplayReport) {
    let entries: Vec<JsonEntry> = r
        .entries
        .iter()
        .map(|e| JsonEntry {
            seq: e.seq,
            tool_id: &e.tool_id,
            verdict: match &e.verdict {
                Verdict::Match => JsonVerdict::Match,
                Verdict::Drift { recorded, actual } => JsonVerdict::Drift { recorded, actual },
                Verdict::HostError(m) => JsonVerdict::HostError { message: m },
                Verdict::Skipped => JsonVerdict::Skipped,
            },
        })
        .collect();
    let report = JsonReport {
        total: r.entries.len(),
        matches: r.matches,
        drifts: r.drifts,
        host_errors: r.host_errors,
        skipped: r.skipped,
        fully_match: r.is_fully_match(),
        bit_identical: is_bit_identical(r),
        entries,
    };
    #[allow(clippy::expect_used)]
    let s = serde_json::to_string_pretty(&report).expect("json report should always serialize");
    println!("{s}");
}
