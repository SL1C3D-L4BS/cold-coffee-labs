//! Wave 9D — `build.*`, `runtime.*`, `code.*`, `vscript.*`, `debug.*` tool
//! dispatchers, plus the scripted hot-reload orchestrator.
//!
//! The strategy follows the same pattern as [`crate::BridgeDispatcher`]:
//!
//! 1. Each family is fronted by a trait (e.g. [`BuildExecutor`]) so
//!    tests can plug in deterministic mocks.
//! 2. A dedicated dispatcher maps tool ids to trait calls.
//! 3. `CompositeDispatcher` (in [`crate::composite`]) routes prefixes to
//!    the right dispatcher.
//!
//! Implementations provided here:
//!
//! * [`BuildDispatcher`] — `build.compile`, `build.clean`, `build.status`,
//!   `build.run_tests`.
//! * [`CodeDispatcher`] — `code.read`, `code.apply_patch`, `code.format`,
//!   `code.search`.
//! * [`VscriptDispatcher`] — `vscript.compile`, `vscript.eval`,
//!   `vscript.list`.
//! * [`RuntimeDispatcher`] — `runtime.step`, `runtime.log_tail`,
//!   `runtime.hot_reload`.
//! * [`DebugDispatcher`] — `debug.snapshot`, `debug.break`, `debug.eval`.
//! * [`ScriptedHotReload`] — end-to-end orchestrator that chains
//!   `code.apply_patch` → `build.compile` → `runtime.hot_reload` →
//!   verification, matching the Phase-9D acceptance test.
//!
//! All dispatchers are **sync** — they execute on the editor main thread
//! or on a dedicated `tokio::task::spawn_blocking` thread when the caller
//! needs to bridge to async. This mirrors the editor's single-writer
//! threading model (CLAUDE.md §3).

use std::collections::HashMap;
use std::path::PathBuf;
use std::sync::Mutex;

use serde::{Deserialize, Serialize};
use serde_json::{json, Value};

use crate::ToolDispatcher;

// =============================================================================
// Traits
// =============================================================================

/// Build-system seam. The real implementation shells out to CMake + Ninja;
/// tests use [`MockBuildExecutor`].
pub trait BuildExecutor: Send + Sync {
    /// Compile the target; return a build id and summary.
    fn compile(&self, target: &str, config: BuildConfig) -> Result<BuildResult, String>;
    /// Clean the target.
    fn clean(&self, target: &str) -> Result<(), String>;
    /// Read last build status.
    fn status(&self) -> Result<BuildStatus, String>;
    /// Run the engine's test suite and capture the summary.
    fn run_tests(&self, filter: Option<&str>) -> Result<TestSummary, String>;
}

/// Build configuration options.
#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct BuildConfig {
    /// `"debug"` or `"release"`.
    #[serde(default = "default_profile")]
    pub profile: String,
    /// Extra CMake defines, e.g. `-DFOO=BAR`.
    #[serde(default)]
    pub defines: Vec<String>,
}

fn default_profile() -> String {
    "debug".into()
}

impl Default for BuildConfig {
    fn default() -> Self {
        Self {
            profile: "debug".into(),
            defines: Vec::new(),
        }
    }
}

/// Build result payload.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BuildResult {
    /// Build id (ULID).
    pub id: String,
    /// Success flag.
    pub ok: bool,
    /// Human-readable summary.
    pub summary: String,
    /// Warnings count.
    pub warnings: u32,
    /// Errors count.
    pub errors: u32,
    /// Paths to the build artifacts, if any.
    pub artifacts: Vec<String>,
}

/// Build status snapshot.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BuildStatus {
    /// Last build id.
    pub last_id: Option<String>,
    /// Is a build currently running?
    pub in_progress: bool,
    /// Last build ok flag.
    pub last_ok: Option<bool>,
}

/// Test-run summary.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TestSummary {
    /// Total tests executed.
    pub total: u32,
    /// Passed.
    pub passed: u32,
    /// Failed.
    pub failed: u32,
    /// Skipped.
    pub skipped: u32,
    /// Failure names (empty on success).
    pub failures: Vec<String>,
}

/// Filesystem seam for `code.*` tools.
pub trait CodeHost: Send + Sync {
    /// Read a file; `start_line`/`end_line` are 1-based and inclusive.
    fn read(
        &self,
        path: &str,
        start_line: Option<u32>,
        end_line: Option<u32>,
    ) -> Result<String, String>;
    /// Apply a unified-diff patch. Returns number of hunks applied.
    fn apply_patch(&self, patch: &str) -> Result<PatchReport, String>;
    /// Format a file (clang-format for C++, rustfmt for Rust, etc.).
    fn format(&self, path: &str) -> Result<(), String>;
    /// Plain-text search under a prefix path. Limit caps results.
    fn search(&self, query: &str, prefix: Option<&str>, limit: u32)
        -> Result<Vec<CodeHit>, String>;
}

/// Patch-application report.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PatchReport {
    /// Files touched.
    pub files: Vec<String>,
    /// Hunks applied.
    pub hunks_applied: u32,
    /// Hunks rejected.
    pub hunks_rejected: u32,
}

/// One text-search hit.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CodeHit {
    /// Path relative to project root.
    pub path: String,
    /// 1-based line number.
    pub line: u32,
    /// The full line.
    pub snippet: String,
}

/// Visual-script host seam.
pub trait VscriptHost: Send + Sync {
    /// Compile a `.vscript` graph; returns a compiled-graph id.
    fn compile(&self, path: &str) -> Result<String, String>;
    /// Evaluate a compiled graph synchronously.
    fn eval(&self, id: &str, inputs: &Value) -> Result<Value, String>;
    /// List known scripts.
    fn list(&self) -> Result<Vec<String>, String>;
}

/// Runtime (PIE) observer seam.
pub trait RuntimeHost: Send + Sync {
    /// Advance simulation by `dt` seconds.
    fn step(&self, dt: f32) -> Result<(), String>;
    /// Tail the log. `max_lines` caps output.
    fn log_tail(&self, max_lines: u32) -> Result<Vec<String>, String>;
    /// Hot-reload gameplay DLL; returns new DLL version.
    fn hot_reload(&self) -> Result<String, String>;
}

/// Debugger seam.
pub trait DebugHost: Send + Sync {
    /// Take a world snapshot (opaque JSON).
    fn snapshot(&self) -> Result<Value, String>;
    /// Trigger a debugger break (pause PIE + raise UI).
    fn break_now(&self) -> Result<(), String>;
    /// Evaluate an expression against the running game.
    fn eval(&self, expr: &str) -> Result<Value, String>;
}

// =============================================================================
// Mock implementations
// =============================================================================

/// Deterministic in-memory build executor.
#[derive(Debug, Default)]
pub struct MockBuildExecutor {
    inner: Mutex<MockBuildState>,
}

#[derive(Debug, Default)]
struct MockBuildState {
    counter: u32,
    last: Option<BuildStatus>,
    test_behaviour: Option<TestSummary>,
    compile_should_fail: bool,
}

impl MockBuildExecutor {
    /// New empty executor.
    pub fn new() -> Self {
        Self::default()
    }

    /// Configure the next [`run_tests`] call's result.
    #[allow(clippy::expect_used)]
    pub fn set_test_result(&self, summary: TestSummary) {
        self.inner
            .lock()
            .expect("mock build mutex poisoned")
            .test_behaviour = Some(summary);
    }

    /// Force the next [`compile`] call to report a failure.
    #[allow(clippy::expect_used)]
    pub fn set_compile_fail(&self, fail: bool) {
        self.inner
            .lock()
            .expect("mock build mutex poisoned")
            .compile_should_fail = fail;
    }
}

impl BuildExecutor for MockBuildExecutor {
    fn compile(&self, target: &str, _config: BuildConfig) -> Result<BuildResult, String> {
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock build mutex poisoned");
        st.counter += 1;
        let id = format!("build-{:04}", st.counter);
        let fail = st.compile_should_fail;
        let result = BuildResult {
            id: id.clone(),
            ok: !fail,
            summary: if fail {
                format!("compile FAILED for `{target}`")
            } else {
                format!("compiled `{target}` ok")
            },
            warnings: 0,
            errors: if fail { 1 } else { 0 },
            artifacts: if fail {
                Vec::new()
            } else {
                vec![format!("build/{target}.dll")]
            },
        };
        st.last = Some(BuildStatus {
            last_id: Some(id),
            in_progress: false,
            last_ok: Some(result.ok),
        });
        Ok(result)
    }

    fn clean(&self, _target: &str) -> Result<(), String> {
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock build mutex poisoned");
        st.last = None;
        st.counter = 0;
        Ok(())
    }

    fn status(&self) -> Result<BuildStatus, String> {
        #[allow(clippy::expect_used)]
        let st = self.inner.lock().expect("mock build mutex poisoned");
        Ok(st.last.clone().unwrap_or(BuildStatus {
            last_id: None,
            in_progress: false,
            last_ok: None,
        }))
    }

    fn run_tests(&self, _filter: Option<&str>) -> Result<TestSummary, String> {
        #[allow(clippy::expect_used)]
        let st = self.inner.lock().expect("mock build mutex poisoned");
        Ok(st.test_behaviour.clone().unwrap_or(TestSummary {
            total: 1,
            passed: 1,
            failed: 0,
            skipped: 0,
            failures: Vec::new(),
        }))
    }
}

/// In-memory code host backed by a hash map.
#[derive(Debug, Default)]
pub struct MockCodeHost {
    inner: Mutex<MockCodeState>,
}

#[derive(Debug, Default)]
struct MockCodeState {
    files: HashMap<String, String>,
    formatted: Vec<String>,
}

impl MockCodeHost {
    /// New empty host.
    pub fn new() -> Self {
        Self::default()
    }

    /// Seed a file into the mock.
    pub fn put(&self, path: &str, contents: &str) {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock code mutex poisoned")
            .files
            .insert(path.to_owned(), contents.to_owned());
    }

    /// Snapshot a file.
    pub fn get(&self, path: &str) -> Option<String> {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock code mutex poisoned")
            .files
            .get(path)
            .cloned()
    }

    /// List files that were formatted (in order).
    pub fn formatted_paths(&self) -> Vec<String> {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock code mutex poisoned")
            .formatted
            .clone()
    }
}

impl CodeHost for MockCodeHost {
    fn read(
        &self,
        path: &str,
        start_line: Option<u32>,
        end_line: Option<u32>,
    ) -> Result<String, String> {
        let contents = self
            .get(path)
            .ok_or_else(|| format!("no such file: {path}"))?;
        if start_line.is_none() && end_line.is_none() {
            return Ok(contents);
        }
        let lines: Vec<&str> = contents.lines().collect();
        let s = start_line.unwrap_or(1).saturating_sub(1) as usize;
        let e = end_line.map(|v| v as usize).unwrap_or(lines.len()).min(lines.len());
        if s >= e {
            return Ok(String::new());
        }
        Ok(lines[s..e].join("\n"))
    }

    fn apply_patch(&self, patch: &str) -> Result<PatchReport, String> {
        // A minimal patch applier for the mock: we expect a sequence of
        // "--- path\n+++ path\n<body>" blocks where <body> is the
        // replacement text delimited by @@ markers. Real code goes through
        // `patch(1)` under the hood, but the mock doesn't need diff
        // fidelity — it needs determinism.
        let mut files = Vec::new();
        let mut hunks = 0u32;

        // Support a simple "@@path:<path>@@<new contents>@@end@@" shape.
        // We split on `@@path:` markers.
        for chunk in patch.split("@@path:").skip(1) {
            let (path, rest) = chunk
                .split_once("@@")
                .ok_or_else(|| "malformed patch: missing @@".to_owned())?;
            let (body, _tail) = rest
                .split_once("@@end@@")
                .ok_or_else(|| "malformed patch: missing @@end@@".to_owned())?;
            let path = path.trim();
            #[allow(clippy::expect_used)]
            self.inner
                .lock()
                .expect("mock code mutex poisoned")
                .files
                .insert(path.to_owned(), body.to_owned());
            files.push(path.to_owned());
            hunks += 1;
        }

        Ok(PatchReport {
            files,
            hunks_applied: hunks,
            hunks_rejected: 0,
        })
    }

    fn format(&self, path: &str) -> Result<(), String> {
        let _ = self.get(path).ok_or_else(|| format!("no such file: {path}"))?;
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock code mutex poisoned")
            .formatted
            .push(path.to_owned());
        Ok(())
    }

    fn search(
        &self,
        query: &str,
        prefix: Option<&str>,
        limit: u32,
    ) -> Result<Vec<CodeHit>, String> {
        #[allow(clippy::expect_used)]
        let st = self.inner.lock().expect("mock code mutex poisoned");
        let mut hits = Vec::new();
        for (path, body) in &st.files {
            if let Some(p) = prefix {
                if !path.starts_with(p) {
                    continue;
                }
            }
            for (i, line) in body.lines().enumerate() {
                if line.contains(query) {
                    hits.push(CodeHit {
                        path: path.clone(),
                        line: (i + 1) as u32,
                        snippet: line.to_owned(),
                    });
                    if hits.len() as u32 >= limit {
                        return Ok(hits);
                    }
                }
            }
        }
        Ok(hits)
    }
}

/// Simple vscript host that just stores a script table.
#[derive(Debug, Default)]
pub struct MockVscriptHost {
    inner: Mutex<MockVscriptState>,
}

#[derive(Debug, Default)]
struct MockVscriptState {
    scripts: HashMap<String, String>,
    next_id: u32,
}

impl MockVscriptHost {
    /// New empty host.
    pub fn new() -> Self {
        Self::default()
    }
}

impl VscriptHost for MockVscriptHost {
    fn compile(&self, path: &str) -> Result<String, String> {
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock vscript mutex poisoned");
        st.next_id += 1;
        let id = format!("vscript-{:04}", st.next_id);
        st.scripts.insert(id.clone(), path.to_owned());
        Ok(id)
    }

    fn eval(&self, _id: &str, inputs: &Value) -> Result<Value, String> {
        // Echo inputs under "result"; good enough for the orchestrator
        // test.
        Ok(json!({ "result": inputs.clone() }))
    }

    fn list(&self) -> Result<Vec<String>, String> {
        #[allow(clippy::expect_used)]
        let st = self.inner.lock().expect("mock vscript mutex poisoned");
        Ok(st.scripts.keys().cloned().collect())
    }
}

/// Mock runtime host that accumulates step time and log lines.
#[derive(Debug, Default)]
pub struct MockRuntimeHost {
    inner: Mutex<MockRuntimeState>,
}

#[derive(Debug, Default)]
struct MockRuntimeState {
    sim_time: f32,
    log: Vec<String>,
    reload_count: u32,
}

impl MockRuntimeHost {
    /// New host.
    pub fn new() -> Self {
        Self::default()
    }
    /// Append a log line for tests.
    pub fn log(&self, line: &str) {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock runtime mutex poisoned")
            .log
            .push(line.to_owned());
    }
    /// How many hot-reloads have happened.
    pub fn reload_count(&self) -> u32 {
        #[allow(clippy::expect_used)]
        self.inner.lock().expect("mock runtime mutex poisoned").reload_count
    }
    /// Current accumulated sim time.
    pub fn sim_time(&self) -> f32 {
        #[allow(clippy::expect_used)]
        self.inner.lock().expect("mock runtime mutex poisoned").sim_time
    }
}

impl RuntimeHost for MockRuntimeHost {
    fn step(&self, dt: f32) -> Result<(), String> {
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock runtime mutex poisoned");
        st.sim_time += dt;
        Ok(())
    }
    fn log_tail(&self, max_lines: u32) -> Result<Vec<String>, String> {
        #[allow(clippy::expect_used)]
        let st = self.inner.lock().expect("mock runtime mutex poisoned");
        let n = st.log.len();
        let start = n.saturating_sub(max_lines as usize);
        Ok(st.log[start..].to_vec())
    }
    fn hot_reload(&self) -> Result<String, String> {
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock runtime mutex poisoned");
        st.reload_count += 1;
        Ok(format!("dll-v{}", st.reload_count))
    }
}

/// Mock debug host.
#[derive(Debug, Default)]
pub struct MockDebugHost {
    inner: Mutex<Vec<String>>,
}

impl MockDebugHost {
    /// New host.
    pub fn new() -> Self {
        Self::default()
    }
    /// Journal of calls.
    pub fn journal(&self) -> Vec<String> {
        #[allow(clippy::expect_used)]
        self.inner.lock().expect("mock debug mutex poisoned").clone()
    }
}

impl DebugHost for MockDebugHost {
    fn snapshot(&self) -> Result<Value, String> {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock debug mutex poisoned")
            .push("snapshot".into());
        Ok(json!({ "entities": 0, "frame": 0 }))
    }
    fn break_now(&self) -> Result<(), String> {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock debug mutex poisoned")
            .push("break".into());
        Ok(())
    }
    fn eval(&self, expr: &str) -> Result<Value, String> {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock debug mutex poisoned")
            .push(format!("eval:{expr}"));
        Ok(json!({ "expr": expr, "value": null }))
    }
}

// =============================================================================
// Dispatchers
// =============================================================================

/// Dispatcher for `build.*`.
pub struct BuildDispatcher<E: BuildExecutor> {
    exec: E,
}

impl<E: BuildExecutor> BuildDispatcher<E> {
    /// New dispatcher.
    pub fn new(exec: E) -> Self {
        Self { exec }
    }

    /// Tool ids handled here.
    pub const fn handled_ids() -> &'static [&'static str] {
        &["build.compile", "build.clean", "build.status", "build.run_tests"]
    }
}

impl<E: BuildExecutor> ToolDispatcher for BuildDispatcher<E> {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        match tool_id {
            "build.compile" => {
                #[derive(Deserialize)]
                struct A {
                    #[serde(default = "default_target")]
                    target: String,
                    #[serde(default)]
                    config: BuildConfig,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let r = self.exec.compile(&a.target, a.config)?;
                serde_json::to_value(r).map_err(|e| format!("serialize: {e}"))
            }
            "build.clean" => {
                #[derive(Deserialize)]
                struct A {
                    #[serde(default = "default_target")]
                    target: String,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                self.exec.clean(&a.target)?;
                Ok(json!({ "ok": true }))
            }
            "build.status" => {
                let s = self.exec.status()?;
                serde_json::to_value(s).map_err(|e| format!("serialize: {e}"))
            }
            "build.run_tests" => {
                #[derive(Deserialize, Default)]
                struct A {
                    #[serde(default)]
                    filter: Option<String>,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let s = self.exec.run_tests(a.filter.as_deref())?;
                serde_json::to_value(s).map_err(|e| format!("serialize: {e}"))
            }
            other => Err(format!("build dispatcher: unknown tool id {other}")),
        }
    }
}

fn default_target() -> String {
    "greywater_game".into()
}

/// Dispatcher for `code.*`.
pub struct CodeDispatcher<H: CodeHost> {
    host: H,
}

impl<H: CodeHost> CodeDispatcher<H> {
    /// New dispatcher.
    pub fn new(host: H) -> Self {
        Self { host }
    }

    /// Tool ids handled here.
    pub const fn handled_ids() -> &'static [&'static str] {
        &["code.read", "code.apply_patch", "code.format", "code.search"]
    }
}

impl<H: CodeHost> ToolDispatcher for CodeDispatcher<H> {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        match tool_id {
            "code.read" => {
                #[derive(Deserialize)]
                struct A {
                    path: String,
                    #[serde(default)]
                    start_line: Option<u32>,
                    #[serde(default)]
                    end_line: Option<u32>,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let text = self.host.read(&a.path, a.start_line, a.end_line)?;
                Ok(json!({ "path": a.path, "text": text }))
            }
            "code.apply_patch" => {
                #[derive(Deserialize)]
                struct A {
                    patch: String,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let r = self.host.apply_patch(&a.patch)?;
                serde_json::to_value(r).map_err(|e| format!("serialize: {e}"))
            }
            "code.format" => {
                #[derive(Deserialize)]
                struct A {
                    path: String,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                self.host.format(&a.path)?;
                Ok(json!({ "ok": true, "path": a.path }))
            }
            "code.search" => {
                #[derive(Deserialize)]
                struct A {
                    query: String,
                    #[serde(default)]
                    prefix: Option<String>,
                    #[serde(default = "default_limit")]
                    limit: u32,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let hits = self.host.search(&a.query, a.prefix.as_deref(), a.limit)?;
                Ok(json!({ "hits": hits, "count": hits_len(&hits) }))
            }
            other => Err(format!("code dispatcher: unknown tool id {other}")),
        }
    }
}

fn default_limit() -> u32 {
    50
}

fn hits_len(h: &[CodeHit]) -> u32 {
    h.len() as u32
}

/// Dispatcher for `vscript.*`.
pub struct VscriptDispatcher<H: VscriptHost> {
    host: H,
}

impl<H: VscriptHost> VscriptDispatcher<H> {
    /// New dispatcher.
    pub fn new(host: H) -> Self {
        Self { host }
    }

    /// Tool ids handled here.
    pub const fn handled_ids() -> &'static [&'static str] {
        &["vscript.compile", "vscript.eval", "vscript.list"]
    }
}

impl<H: VscriptHost> ToolDispatcher for VscriptDispatcher<H> {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        match tool_id {
            "vscript.compile" => {
                #[derive(Deserialize)]
                struct A {
                    path: String,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let id = self.host.compile(&a.path)?;
                Ok(json!({ "ok": true, "id": id }))
            }
            "vscript.eval" => {
                #[derive(Deserialize)]
                struct A {
                    id: String,
                    #[serde(default)]
                    inputs: Value,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let out = self.host.eval(&a.id, &a.inputs)?;
                Ok(json!({ "ok": true, "output": out }))
            }
            "vscript.list" => {
                let xs = self.host.list()?;
                Ok(json!({ "scripts": xs }))
            }
            other => Err(format!("vscript dispatcher: unknown tool id {other}")),
        }
    }
}

/// Dispatcher for `runtime.*` (excluding `runtime.enter_play`/`exit_play`,
/// which go through [`crate::BridgeDispatcher`]).
pub struct RuntimeDispatcher<H: RuntimeHost> {
    host: H,
}

impl<H: RuntimeHost> RuntimeDispatcher<H> {
    /// New dispatcher.
    pub fn new(host: H) -> Self {
        Self { host }
    }

    /// Tool ids handled here.
    pub const fn handled_ids() -> &'static [&'static str] {
        &["runtime.step", "runtime.log_tail", "runtime.hot_reload"]
    }
}

impl<H: RuntimeHost> ToolDispatcher for RuntimeDispatcher<H> {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        match tool_id {
            "runtime.step" => {
                #[derive(Deserialize)]
                struct A {
                    #[serde(default = "default_dt")]
                    dt: f32,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                self.host.step(a.dt)?;
                Ok(json!({ "ok": true, "dt": a.dt }))
            }
            "runtime.log_tail" => {
                #[derive(Deserialize, Default)]
                struct A {
                    #[serde(default = "default_max_lines")]
                    max_lines: u32,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                let lines = self.host.log_tail(a.max_lines)?;
                Ok(json!({ "lines": lines }))
            }
            "runtime.hot_reload" => {
                let v = self.host.hot_reload()?;
                Ok(json!({ "ok": true, "version": v }))
            }
            other => Err(format!("runtime dispatcher: unknown tool id {other}")),
        }
    }
}

fn default_dt() -> f32 {
    1.0 / 60.0
}
fn default_max_lines() -> u32 {
    64
}

/// Dispatcher for `debug.*`.
pub struct DebugDispatcher<H: DebugHost> {
    host: H,
}

impl<H: DebugHost> DebugDispatcher<H> {
    /// New dispatcher.
    pub fn new(host: H) -> Self {
        Self { host }
    }

    /// Tool ids handled here.
    pub const fn handled_ids() -> &'static [&'static str] {
        &["debug.snapshot", "debug.break", "debug.eval"]
    }
}

impl<H: DebugHost> ToolDispatcher for DebugDispatcher<H> {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        match tool_id {
            "debug.snapshot" => self.host.snapshot(),
            "debug.break" => {
                self.host.break_now()?;
                Ok(json!({ "ok": true }))
            }
            "debug.eval" => {
                #[derive(Deserialize)]
                struct A {
                    expr: String,
                }
                let a: A =
                    serde_json::from_value(args.clone()).map_err(|e| format!("bad input: {e}"))?;
                self.host.eval(&a.expr)
            }
            other => Err(format!("debug dispatcher: unknown tool id {other}")),
        }
    }
}

// =============================================================================
// ScriptedHotReload — end-to-end acceptance orchestrator (ADR-0014 §4.1)
// =============================================================================

/// End-to-end orchestrator for the Phase-9D acceptance scenario:
///
/// ```text
/// 1. code.apply_patch  (edit a header)
/// 2. build.compile     (must succeed)
/// 3. runtime.hot_reload (reload the DLL)
/// 4. verify            (tester-supplied closure over the runtime host)
/// ```
///
/// If any step fails the orchestrator returns an error containing the
/// step name and the inner error. The [`HotReloadReport`] records what
/// happened per step for the audit log.
pub struct ScriptedHotReload<
    'a,
    E: BuildExecutor,
    C: CodeHost,
    R: RuntimeHost,
> {
    /// Build executor.
    pub build: &'a E,
    /// Code host.
    pub code: &'a C,
    /// Runtime host.
    pub runtime: &'a R,
}

/// Outcome of a scripted hot-reload run.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HotReloadReport {
    /// Files touched by the patch.
    pub patched_files: Vec<String>,
    /// Build result id.
    pub build_id: String,
    /// DLL version after reload.
    pub dll_version: String,
    /// Whether verify succeeded.
    pub verified: bool,
    /// Target that was rebuilt.
    pub target: String,
}

impl<E: BuildExecutor, C: CodeHost, R: RuntimeHost> ScriptedHotReload<'_, E, C, R> {
    /// Run the full sequence.
    pub fn run<V>(
        &self,
        patch: &str,
        target: &str,
        mut verify: V,
    ) -> Result<HotReloadReport, String>
    where
        V: FnMut(&R) -> Result<bool, String>,
    {
        let patch_report = self
            .code
            .apply_patch(patch)
            .map_err(|e| format!("code.apply_patch: {e}"))?;

        let build = self
            .build
            .compile(target, BuildConfig::default())
            .map_err(|e| format!("build.compile: {e}"))?;
        if !build.ok {
            return Err(format!(
                "build.compile failed ({} errors): {}",
                build.errors, build.summary
            ));
        }

        let dll = self
            .runtime
            .hot_reload()
            .map_err(|e| format!("runtime.hot_reload: {e}"))?;

        let ok = verify(self.runtime).map_err(|e| format!("verify: {e}"))?;

        Ok(HotReloadReport {
            patched_files: patch_report.files,
            build_id: build.id,
            dll_version: dll,
            verified: ok,
            target: target.to_owned(),
        })
    }
}

// =============================================================================
// Project-root aware `CodeHost` (production)
// =============================================================================

/// Filesystem-backed `CodeHost` rooted at `root`. Reads through
/// [`SecretFilter`] to prevent exfiltrating secrets.
///
/// `apply_patch` is deliberately stubbed until Phase 9D Plus lands the
/// real unified-diff machinery: we don't want to land a half-working
/// patch applier in the same PR as the dispatcher scaffolding.
///
/// [`SecretFilter`]: bld_governance::secret_filter::SecretFilter
pub struct FsCodeHost {
    /// Project root.
    pub root: PathBuf,
    secrets: bld_governance::secret_filter::SecretFilter,
}

impl FsCodeHost {
    /// New host rooted at `root` with the default secret policy.
    pub fn new(root: impl Into<PathBuf>) -> Self {
        Self {
            root: root.into(),
            secrets: bld_governance::secret_filter::SecretFilter::new(),
        }
    }
}

impl CodeHost for FsCodeHost {
    fn read(
        &self,
        path: &str,
        start_line: Option<u32>,
        end_line: Option<u32>,
    ) -> Result<String, String> {
        let full = self.root.join(path);
        if let Err(hit) = self.secrets.check(&full) {
            return Err(format!("secret filter: {} ({:?})", hit.path, hit.reason));
        }
        let bytes = std::fs::read_to_string(&full).map_err(|e| format!("read {path}: {e}"))?;
        if start_line.is_none() && end_line.is_none() {
            return Ok(bytes);
        }
        let lines: Vec<&str> = bytes.lines().collect();
        let s = start_line.unwrap_or(1).saturating_sub(1) as usize;
        let e = end_line.map(|v| v as usize).unwrap_or(lines.len()).min(lines.len());
        if s >= e {
            return Ok(String::new());
        }
        Ok(lines[s..e].join("\n"))
    }

    fn apply_patch(&self, _patch: &str) -> Result<PatchReport, String> {
        Err("FsCodeHost.apply_patch: not yet implemented (ADR-0014 §4.2)".into())
    }

    fn format(&self, _path: &str) -> Result<(), String> {
        Err("FsCodeHost.format: not yet implemented (ADR-0014 §4.2)".into())
    }

    fn search(
        &self,
        _query: &str,
        _prefix: Option<&str>,
        _limit: u32,
    ) -> Result<Vec<CodeHit>, String> {
        Err("FsCodeHost.search: not yet implemented (ADR-0014 §4.2)".into())
    }
}

// =============================================================================
// Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn build_dispatcher_compiles_and_reports() {
        let d = BuildDispatcher::new(MockBuildExecutor::new());
        let r = d
            .dispatch(
                "build.compile",
                &json!({ "target": "greywater_game", "config": { "profile": "debug" } }),
            )
            .unwrap();
        assert_eq!(r.get("ok").unwrap(), true);
        assert_eq!(r.get("errors").unwrap(), 0);
        let s = d.dispatch("build.status", &json!({})).unwrap();
        assert_eq!(s.get("last_ok").unwrap(), true);
    }

    #[test]
    fn build_dispatcher_surfaces_failure() {
        let exec = MockBuildExecutor::new();
        exec.set_compile_fail(true);
        let d = BuildDispatcher::new(exec);
        let r = d
            .dispatch("build.compile", &json!({ "target": "x" }))
            .unwrap();
        assert_eq!(r.get("ok").unwrap(), false);
        assert_eq!(r.get("errors").unwrap(), 1);
    }

    #[test]
    fn build_dispatcher_runs_tests() {
        let exec = MockBuildExecutor::new();
        exec.set_test_result(TestSummary {
            total: 3,
            passed: 2,
            failed: 1,
            skipped: 0,
            failures: vec!["integration::foo".into()],
        });
        let d = BuildDispatcher::new(exec);
        let r = d.dispatch("build.run_tests", &json!({})).unwrap();
        assert_eq!(r.get("failed").unwrap(), 1);
        assert_eq!(r.get("failures").unwrap().as_array().unwrap().len(), 1);
    }

    #[test]
    fn code_dispatcher_read_and_patch() {
        let host = MockCodeHost::new();
        host.put("src/foo.rs", "line 1\nline 2\nline 3\n");
        let d = CodeDispatcher::new(host);
        let r = d
            .dispatch("code.read", &json!({ "path": "src/foo.rs" }))
            .unwrap();
        assert!(r.get("text").unwrap().as_str().unwrap().contains("line 1"));

        let patch = "@@path:src/foo.rs@@line A\nline B\n@@end@@";
        let rep = d
            .dispatch("code.apply_patch", &json!({ "patch": patch }))
            .unwrap();
        assert_eq!(rep.get("hunks_applied").unwrap(), 1);

        let r2 = d
            .dispatch("code.read", &json!({ "path": "src/foo.rs" }))
            .unwrap();
        assert!(r2.get("text").unwrap().as_str().unwrap().contains("line A"));
    }

    #[test]
    fn code_dispatcher_search_respects_prefix_and_limit() {
        let host = MockCodeHost::new();
        host.put("src/a.rs", "hello world\nhello there\n");
        host.put("tests/a.rs", "hello tests\n");
        let d = CodeDispatcher::new(host);
        let r = d
            .dispatch(
                "code.search",
                &json!({ "query": "hello", "prefix": "src/", "limit": 10 }),
            )
            .unwrap();
        assert_eq!(r.get("count").unwrap(), 2);

        let r2 = d
            .dispatch(
                "code.search",
                &json!({ "query": "hello", "limit": 1 }),
            )
            .unwrap();
        assert_eq!(r2.get("count").unwrap(), 1);
    }

    #[test]
    fn code_read_slices_by_lines() {
        let host = MockCodeHost::new();
        host.put("x.rs", "1\n2\n3\n4\n5\n");
        let d = CodeDispatcher::new(host);
        let r = d
            .dispatch(
                "code.read",
                &json!({ "path": "x.rs", "start_line": 2, "end_line": 4 }),
            )
            .unwrap();
        assert_eq!(r.get("text").unwrap().as_str().unwrap(), "2\n3\n4");
    }

    #[test]
    fn vscript_dispatcher_compile_and_eval() {
        let d = VscriptDispatcher::new(MockVscriptHost::new());
        let c = d
            .dispatch("vscript.compile", &json!({ "path": "scripts/door.vscript" }))
            .unwrap();
        let id = c.get("id").unwrap().as_str().unwrap().to_owned();
        let e = d
            .dispatch(
                "vscript.eval",
                &json!({ "id": id, "inputs": {"x": 1} }),
            )
            .unwrap();
        assert_eq!(e.get("output").unwrap().get("result").unwrap().get("x").unwrap(), 1);
    }

    #[test]
    fn runtime_dispatcher_step_log_reload() {
        let host = MockRuntimeHost::new();
        host.log("hello");
        host.log("world");
        let d = RuntimeDispatcher::new(host);
        d.dispatch("runtime.step", &json!({ "dt": 0.25 })).unwrap();
        let t = d.dispatch("runtime.log_tail", &json!({ "max_lines": 1 })).unwrap();
        assert_eq!(t.get("lines").unwrap().as_array().unwrap().len(), 1);
        let r = d.dispatch("runtime.hot_reload", &json!({})).unwrap();
        assert_eq!(r.get("version").unwrap().as_str().unwrap(), "dll-v1");
    }

    #[test]
    fn debug_dispatcher_snapshot_break_eval() {
        let host = MockDebugHost::new();
        let d = DebugDispatcher::new(host);
        d.dispatch("debug.snapshot", &json!({})).unwrap();
        d.dispatch("debug.break", &json!({})).unwrap();
        let e = d
            .dispatch("debug.eval", &json!({ "expr": "player.hp" }))
            .unwrap();
        assert_eq!(e.get("expr").unwrap().as_str().unwrap(), "player.hp");
    }

    #[test]
    fn scripted_hot_reload_end_to_end() {
        let build = MockBuildExecutor::new();
        let code = MockCodeHost::new();
        code.put("gameplay.rs", "old\n");
        let runtime = MockRuntimeHost::new();

        let orch = ScriptedHotReload {
            build: &build,
            code: &code,
            runtime: &runtime,
        };
        let patch = "@@path:gameplay.rs@@new\n@@end@@";
        let report = orch
            .run(patch, "greywater_game", |rt| Ok(rt.reload_count() == 1))
            .unwrap();
        assert_eq!(report.patched_files, vec!["gameplay.rs".to_string()]);
        assert!(report.verified);
        assert_eq!(report.dll_version, "dll-v1");
        assert_eq!(code.get("gameplay.rs").as_deref(), Some("new\n"));
    }

    #[test]
    fn scripted_hot_reload_short_circuits_on_build_failure() {
        let build = MockBuildExecutor::new();
        build.set_compile_fail(true);
        let code = MockCodeHost::new();
        code.put("gameplay.rs", "old\n");
        let runtime = MockRuntimeHost::new();

        let orch = ScriptedHotReload {
            build: &build,
            code: &code,
            runtime: &runtime,
        };
        let patch = "@@path:gameplay.rs@@oops\n@@end@@";
        let err = orch
            .run(patch, "greywater_game", |_| Ok(true))
            .unwrap_err();
        assert!(err.contains("build.compile failed"));
        assert_eq!(runtime.reload_count(), 0, "no reload on build failure");
    }

    #[test]
    fn handled_ids_cover_expected_tools() {
        assert!(BuildDispatcher::<MockBuildExecutor>::handled_ids().contains(&"build.compile"));
        assert!(CodeDispatcher::<MockCodeHost>::handled_ids().contains(&"code.apply_patch"));
        assert!(VscriptDispatcher::<MockVscriptHost>::handled_ids().contains(&"vscript.compile"));
        assert!(RuntimeDispatcher::<MockRuntimeHost>::handled_ids().contains(&"runtime.hot_reload"));
        assert!(DebugDispatcher::<MockDebugHost>::handled_ids().contains(&"debug.snapshot"));
    }
}
