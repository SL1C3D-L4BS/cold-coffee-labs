//! Bridge-backed dispatcher for mutating tools (Wave 9B).
//!
//! The pipeline for a mutating tool call:
//!
//! ```text
//! Agent -> governance.require() -> BridgeDispatcher.dispatch() ->
//!   HostBridge callback -> Surface P v2 run_command (or direct entry) ->
//!   editor CommandStack push -> typed result back to the agent.
//! ```
//!
//! The `HostBridge` trait is the seam. In production `bld-agent` ships
//! the `FfiHostBridge` implementation, which routes through
//! `bld_bridge::{create_entity, destroy_entity, set_field, …}`. Tests
//! use `MockHostBridge` which records every call and can be introspected.
//!
//! All entries in this dispatcher:
//! - Are synchronous (the editor main thread is the execution context).
//! - Push onto the editor `CommandStack` (via Surface P v2 `run_command`
//!   or directly via the typed wrappers). Undo is therefore free.
//! - Return a JSON payload shaped like `{ ok: true, handle: 123 }`.
//!
//! The dispatcher covers the `scene.*`, `component.*`, and `editor.*`
//! mutating families. Read-tier tools in the same families are handled
//! by the `RagDispatcher` where the index is authoritative; write-tier
//! tools need the editor, so they live here.

use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::sync::Mutex;

use crate::ToolDispatcher;

/// Anything the agent needs the editor to do. Trait form so tests can
/// plug in without touching the FFI.
pub trait HostBridge: Send + Sync {
    /// Create an entity and return its handle.
    fn create_entity(&self, name: Option<&str>) -> Result<u64, String>;
    /// Destroy an entity.
    fn destroy_entity(&self, handle: u64) -> Result<(), String>;
    /// Rename an entity. Maps to `set_field(handle, "NameComponent.name", "…")`.
    fn rename_entity(&self, handle: u64, new_name: &str) -> Result<(), String>;
    /// Set a component field by reflection path.
    fn set_field(&self, handle: u64, path: &str, value_json: &str) -> Result<(), String>;
    /// Read a component field (JSON scalar or object encoded as string).
    fn get_field(&self, handle: u64, path: &str) -> Result<String, String>;
    /// Add a component by type name.
    fn add_component(&self, handle: u64, type_name: &str) -> Result<(), String>;
    /// Remove a component by type name.
    fn remove_component(&self, handle: u64, type_name: &str) -> Result<(), String>;
    /// Undo top of stack.
    fn undo(&self) -> Result<bool, String>;
    /// Redo top of stack.
    fn redo(&self) -> Result<bool, String>;
    /// Save active scene.
    fn save_scene(&self, path: &str) -> Result<(), String>;
    /// Load a scene.
    fn load_scene(&self, path: &str) -> Result<(), String>;
    /// Enter PIE.
    fn enter_play(&self) -> Result<(), String>;
    /// Exit PIE.
    fn exit_play(&self) -> Result<(), String>;
}

/// Simple in-memory bridge for tests and the default no-editor path.
///
/// `MockHostBridge` is a faithful-enough reference implementation that
/// the agent-level tests can verify command dispatch without a real
/// editor. Every call appends to an internal journal; the test asserts
/// against it via [`MockHostBridge::journal`].
#[derive(Debug, Default)]
pub struct MockHostBridge {
    inner: Mutex<MockState>,
}

#[derive(Debug, Default)]
struct MockState {
    next_handle: u64,
    journal: Vec<BridgeCall>,
    entities: Vec<MockEntity>,
}

#[derive(Debug, Clone)]
struct MockEntity {
    handle: u64,
    name: String,
    fields: Vec<(String, String)>,
    components: Vec<String>,
    alive: bool,
}

/// Record of a call made to a [`MockHostBridge`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum BridgeCall {
    /// `create_entity`.
    CreateEntity(Option<String>),
    /// `destroy_entity`.
    DestroyEntity(u64),
    /// `rename_entity`.
    RenameEntity(u64, String),
    /// `set_field`.
    SetField(u64, String, String),
    /// `get_field`.
    GetField(u64, String),
    /// `add_component`.
    AddComponent(u64, String),
    /// `remove_component`.
    RemoveComponent(u64, String),
    /// `undo`.
    Undo,
    /// `redo`.
    Redo,
    /// `save_scene`.
    SaveScene(String),
    /// `load_scene`.
    LoadScene(String),
    /// `enter_play`.
    EnterPlay,
    /// `exit_play`.
    ExitPlay,
}

impl MockHostBridge {
    /// New empty bridge.
    pub fn new() -> Self {
        Self::default()
    }

    /// Snapshot the call journal.
    pub fn journal(&self) -> Vec<BridgeCall> {
        #[allow(clippy::expect_used)]
        self.inner.lock().expect("mock mutex poisoned").journal.clone()
    }

    /// Number of currently-live entities.
    pub fn entity_count(&self) -> usize {
        #[allow(clippy::expect_used)]
        self.inner
            .lock()
            .expect("mock mutex poisoned")
            .entities
            .iter()
            .filter(|e| e.alive)
            .count()
    }

    fn push(&self, call: BridgeCall) {
        #[allow(clippy::expect_used)]
        self.inner.lock().expect("mock mutex poisoned").journal.push(call);
    }
}

impl HostBridge for MockHostBridge {
    fn create_entity(&self, name: Option<&str>) -> Result<u64, String> {
        self.push(BridgeCall::CreateEntity(name.map(str::to_owned)));
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock mutex poisoned");
        st.next_handle += 1;
        let handle = st.next_handle;
        st.entities.push(MockEntity {
            handle,
            name: name.unwrap_or("Entity").to_owned(),
            fields: Vec::new(),
            components: Vec::new(),
            alive: true,
        });
        Ok(handle)
    }

    fn destroy_entity(&self, handle: u64) -> Result<(), String> {
        self.push(BridgeCall::DestroyEntity(handle));
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock mutex poisoned");
        let e = st
            .entities
            .iter_mut()
            .find(|e| e.handle == handle && e.alive)
            .ok_or_else(|| format!("unknown handle {handle}"))?;
        e.alive = false;
        Ok(())
    }

    fn rename_entity(&self, handle: u64, new_name: &str) -> Result<(), String> {
        self.push(BridgeCall::RenameEntity(handle, new_name.to_owned()));
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock mutex poisoned");
        let e = st
            .entities
            .iter_mut()
            .find(|e| e.handle == handle && e.alive)
            .ok_or_else(|| format!("unknown handle {handle}"))?;
        e.name = new_name.to_owned();
        Ok(())
    }

    fn set_field(&self, handle: u64, path: &str, value_json: &str) -> Result<(), String> {
        self.push(BridgeCall::SetField(
            handle,
            path.to_owned(),
            value_json.to_owned(),
        ));
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock mutex poisoned");
        let e = st
            .entities
            .iter_mut()
            .find(|e| e.handle == handle && e.alive)
            .ok_or_else(|| format!("unknown handle {handle}"))?;
        if let Some(slot) = e.fields.iter_mut().find(|(k, _)| k == path) {
            slot.1 = value_json.to_owned();
        } else {
            e.fields.push((path.to_owned(), value_json.to_owned()));
        }
        Ok(())
    }

    fn get_field(&self, handle: u64, path: &str) -> Result<String, String> {
        self.push(BridgeCall::GetField(handle, path.to_owned()));
        #[allow(clippy::expect_used)]
        let st = self.inner.lock().expect("mock mutex poisoned");
        let e = st
            .entities
            .iter()
            .find(|e| e.handle == handle && e.alive)
            .ok_or_else(|| format!("unknown handle {handle}"))?;
        e.fields
            .iter()
            .find(|(k, _)| k == path)
            .map(|(_, v)| v.clone())
            .ok_or_else(|| format!("unknown field {path}"))
    }

    fn add_component(&self, handle: u64, type_name: &str) -> Result<(), String> {
        self.push(BridgeCall::AddComponent(handle, type_name.to_owned()));
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock mutex poisoned");
        let e = st
            .entities
            .iter_mut()
            .find(|e| e.handle == handle && e.alive)
            .ok_or_else(|| format!("unknown handle {handle}"))?;
        if !e.components.iter().any(|c| c == type_name) {
            e.components.push(type_name.to_owned());
        }
        Ok(())
    }

    fn remove_component(&self, handle: u64, type_name: &str) -> Result<(), String> {
        self.push(BridgeCall::RemoveComponent(handle, type_name.to_owned()));
        #[allow(clippy::expect_used)]
        let mut st = self.inner.lock().expect("mock mutex poisoned");
        let e = st
            .entities
            .iter_mut()
            .find(|e| e.handle == handle && e.alive)
            .ok_or_else(|| format!("unknown handle {handle}"))?;
        e.components.retain(|c| c != type_name);
        Ok(())
    }

    fn undo(&self) -> Result<bool, String> {
        self.push(BridgeCall::Undo);
        Ok(true)
    }
    fn redo(&self) -> Result<bool, String> {
        self.push(BridgeCall::Redo);
        Ok(true)
    }

    fn save_scene(&self, path: &str) -> Result<(), String> {
        self.push(BridgeCall::SaveScene(path.to_owned()));
        Ok(())
    }

    fn load_scene(&self, path: &str) -> Result<(), String> {
        self.push(BridgeCall::LoadScene(path.to_owned()));
        Ok(())
    }

    fn enter_play(&self) -> Result<(), String> {
        self.push(BridgeCall::EnterPlay);
        Ok(())
    }
    fn exit_play(&self) -> Result<(), String> {
        self.push(BridgeCall::ExitPlay);
        Ok(())
    }
}

/// Routes mutating `scene.*` / `component.*` / `editor.*` tools to a
/// [`HostBridge`].
pub struct BridgeDispatcher<B: HostBridge> {
    bridge: B,
}

impl<B: HostBridge> std::fmt::Debug for BridgeDispatcher<B> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("BridgeDispatcher").finish()
    }
}

impl<B: HostBridge> BridgeDispatcher<B> {
    /// New dispatcher wrapping the given bridge.
    pub fn new(bridge: B) -> Self {
        Self { bridge }
    }

    /// Access the underlying bridge (for bridge-specific queries).
    pub fn bridge(&self) -> &B {
        &self.bridge
    }

    fn route(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        match tool_id {
            // scene.*
            "scene.create_entity" => {
                #[derive(Deserialize)]
                struct A {
                    #[serde(default)]
                    name: Option<String>,
                }
                let a: A = serde_json::from_value(args.clone())
                    .map_err(|e| format!("bad input: {e}"))?;
                let h = self.bridge.create_entity(a.name.as_deref())?;
                Ok(json!({ "ok": true, "handle": h }))
            }
            "scene.delete_entity" => {
                let h = read_handle(args, "handle")?;
                self.bridge.destroy_entity(h)?;
                Ok(json!({ "ok": true }))
            }
            "scene.rename_entity" => {
                #[derive(Deserialize)]
                struct A {
                    handle: u64,
                    name: String,
                }
                let a: A = serde_json::from_value(args.clone())
                    .map_err(|e| format!("bad input: {e}"))?;
                self.bridge.rename_entity(a.handle, &a.name)?;
                Ok(json!({ "ok": true }))
            }
            "scene.save" => {
                let p = read_string(args, "path")?;
                self.bridge.save_scene(&p)?;
                Ok(json!({ "ok": true, "path": p }))
            }
            "scene.load" => {
                let p = read_string(args, "path")?;
                self.bridge.load_scene(&p)?;
                Ok(json!({ "ok": true, "path": p }))
            }
            // component.*
            "component.add" => {
                #[derive(Deserialize)]
                struct A {
                    handle: u64,
                    type_name: String,
                }
                let a: A = serde_json::from_value(args.clone())
                    .map_err(|e| format!("bad input: {e}"))?;
                self.bridge.add_component(a.handle, &a.type_name)?;
                Ok(json!({ "ok": true }))
            }
            "component.remove" => {
                #[derive(Deserialize)]
                struct A {
                    handle: u64,
                    type_name: String,
                }
                let a: A = serde_json::from_value(args.clone())
                    .map_err(|e| format!("bad input: {e}"))?;
                self.bridge.remove_component(a.handle, &a.type_name)?;
                Ok(json!({ "ok": true }))
            }
            "component.set_field" => {
                #[derive(Deserialize)]
                struct A {
                    handle: u64,
                    path: String,
                    value: Value,
                }
                let a: A = serde_json::from_value(args.clone())
                    .map_err(|e| format!("bad input: {e}"))?;
                let value_json = serde_json::to_string(&a.value)
                    .map_err(|e| format!("serialize: {e}"))?;
                self.bridge.set_field(a.handle, &a.path, &value_json)?;
                Ok(json!({ "ok": true }))
            }
            "component.get" => {
                #[derive(Deserialize)]
                struct A {
                    handle: u64,
                    path: String,
                }
                let a: A = serde_json::from_value(args.clone())
                    .map_err(|e| format!("bad input: {e}"))?;
                let raw = self.bridge.get_field(a.handle, &a.path)?;
                // Try to parse as JSON first; fall back to a bare string.
                let parsed: Value =
                    serde_json::from_str(&raw).unwrap_or_else(|_| Value::String(raw));
                Ok(json!({ "ok": true, "value": parsed }))
            }
            // editor.*
            "editor.undo" => {
                let ok = self.bridge.undo()?;
                Ok(json!({ "ok": ok }))
            }
            "editor.redo" => {
                let ok = self.bridge.redo()?;
                Ok(json!({ "ok": ok }))
            }
            // runtime.* (PIE control)
            "runtime.enter_play" => {
                self.bridge.enter_play()?;
                Ok(json!({ "ok": true }))
            }
            "runtime.exit_play" => {
                self.bridge.exit_play()?;
                Ok(json!({ "ok": true }))
            }
            other => Err(format!("tool not handled by bridge dispatcher: {other}")),
        }
    }

    /// Which tool ids this dispatcher handles. Drives the
    /// `CompositeDispatcher` routing table.
    pub const fn handled_ids() -> &'static [&'static str] {
        &[
            "scene.create_entity",
            "scene.delete_entity",
            "scene.rename_entity",
            "scene.save",
            "scene.load",
            "component.add",
            "component.remove",
            "component.set_field",
            "component.get",
            "editor.undo",
            "editor.redo",
            "runtime.enter_play",
            "runtime.exit_play",
        ]
    }
}

impl<B: HostBridge> ToolDispatcher for BridgeDispatcher<B> {
    fn dispatch(&self, tool_id: &str, args: &Value) -> Result<Value, String> {
        self.route(tool_id, args)
    }
}

fn read_handle(args: &Value, key: &str) -> Result<u64, String> {
    args.get(key)
        .and_then(Value::as_u64)
        .ok_or_else(|| format!("missing u64 field `{key}`"))
}

fn read_string(args: &Value, key: &str) -> Result<String, String> {
    args.get(key)
        .and_then(Value::as_str)
        .map(str::to_owned)
        .ok_or_else(|| format!("missing string field `{key}`"))
}

// =============================================================================
// CommandStack abstraction (ADR-0012 §2.4 + ADR-0015 §2.5)
// =============================================================================

/// Typed record of a tool-call that pushed onto the editor CommandStack.
///
/// Agent-initiated mutations wrap each call in a `CommandEntry` so the
/// user sees a single Ctrl-Z-able transaction per turn even if the agent
/// emitted several sub-calls. The Surface P v2 `run_command_batch`
/// primitive is the native mechanism; this type is the Rust-side
/// bookkeeping that maps to it 1:1.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CommandEntry {
    /// Tool id that produced the command.
    pub tool_id: String,
    /// Arguments JSON.
    pub args: Value,
    /// Result JSON.
    pub result: Value,
}

/// Agent-side view of the editor undo/redo stack.
///
/// Each agent turn opens a `CommandStack` transaction; every mutating
/// tool call appends a `CommandEntry`. On turn end the stack either
/// commits (no-op on the editor side, the commands already landed) or
/// unrolls via `undo()` per entry. The editor's CommandStack is
/// authoritative for cross-turn undo; this struct is purely a turn
/// journal for replay and elicitation diffs.
#[derive(Debug, Default, Clone, Serialize, Deserialize)]
pub struct CommandStack {
    /// Entries pushed in this transaction.
    pub entries: Vec<CommandEntry>,
}

impl CommandStack {
    /// New empty stack.
    pub fn new() -> Self {
        Self::default()
    }

    /// Record a mutating tool call.
    pub fn record(&mut self, tool_id: &str, args: Value, result: Value) {
        self.entries.push(CommandEntry {
            tool_id: tool_id.to_owned(),
            args,
            result,
        });
    }

    /// Number of recorded entries.
    pub fn len(&self) -> usize {
        self.entries.len()
    }

    /// Whether the stack is empty.
    pub fn is_empty(&self) -> bool {
        self.entries.is_empty()
    }

    /// Unroll the stack: emit `editor.undo` invocations against the
    /// given [`HostBridge`] in reverse order.
    pub fn unroll<B: HostBridge>(&mut self, bridge: &B) -> Result<usize, String> {
        let mut count = 0usize;
        while self.entries.pop().is_some() {
            bridge.undo()?;
            count += 1;
        }
        Ok(count)
    }
}

// =============================================================================
// Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn disp() -> BridgeDispatcher<MockHostBridge> {
        BridgeDispatcher::new(MockHostBridge::new())
    }

    #[test]
    fn create_entity_returns_handle_and_journal() {
        let d = disp();
        let out = d
            .dispatch("scene.create_entity", &json!({"name": "Cube"}))
            .unwrap();
        assert_eq!(out.get("ok").unwrap(), true);
        let h = out.get("handle").unwrap().as_u64().unwrap();
        assert_eq!(h, 1);
        assert_eq!(d.bridge().entity_count(), 1);
        assert_eq!(d.bridge().journal().len(), 1);
    }

    #[test]
    fn delete_entity_removes_from_count() {
        let d = disp();
        let h = d
            .dispatch("scene.create_entity", &json!({"name": "A"}))
            .unwrap()
            .get("handle")
            .unwrap()
            .as_u64()
            .unwrap();
        d.dispatch("scene.delete_entity", &json!({"handle": h}))
            .unwrap();
        assert_eq!(d.bridge().entity_count(), 0);
    }

    #[test]
    fn component_add_and_set_field_roundtrip() {
        let d = disp();
        let h = d
            .dispatch("scene.create_entity", &json!({"name": "Player"}))
            .unwrap()
            .get("handle")
            .unwrap()
            .as_u64()
            .unwrap();
        d.dispatch(
            "component.add",
            &json!({"handle": h, "type_name": "HealthComponent"}),
        )
        .unwrap();
        d.dispatch(
            "component.set_field",
            &json!({
                "handle": h,
                "path": "HealthComponent.hp",
                "value": 100,
            }),
        )
        .unwrap();
        let got = d
            .dispatch(
                "component.get",
                &json!({"handle": h, "path": "HealthComponent.hp"}),
            )
            .unwrap();
        assert_eq!(got.get("value").unwrap(), 100);
    }

    #[test]
    fn unknown_tool_surfaces_clear_error() {
        let d = disp();
        let err = d.dispatch("scene.teleport", &json!({})).unwrap_err();
        assert!(err.contains("not handled by bridge"));
    }

    #[test]
    fn command_stack_records_and_unrolls() {
        let d = disp();
        let mut stack = CommandStack::new();
        for name in &["A", "B", "C"] {
            let args = json!({"name": name});
            let r = d.dispatch("scene.create_entity", &args).unwrap();
            stack.record("scene.create_entity", args, r);
        }
        assert_eq!(stack.len(), 3);
        assert_eq!(d.bridge().entity_count(), 3);

        let undone = stack.unroll(d.bridge()).unwrap();
        assert_eq!(undone, 3);
        assert!(stack.is_empty());
        let journal = d.bridge().journal();
        // Last three calls in the journal are Undos.
        let undo_count = journal.iter().filter(|c| matches!(c, BridgeCall::Undo)).count();
        assert_eq!(undo_count, 3);
    }

    #[test]
    fn runtime_enter_exit_play_dispatches() {
        let d = disp();
        d.dispatch("runtime.enter_play", &json!({})).unwrap();
        d.dispatch("runtime.exit_play", &json!({})).unwrap();
        let j = d.bridge().journal();
        assert!(j.iter().any(|c| matches!(c, BridgeCall::EnterPlay)));
        assert!(j.iter().any(|c| matches!(c, BridgeCall::ExitPlay)));
    }

    #[test]
    fn editor_undo_redo_pass_through() {
        let d = disp();
        let u = d.dispatch("editor.undo", &json!({})).unwrap();
        let r = d.dispatch("editor.redo", &json!({})).unwrap();
        assert_eq!(u.get("ok").unwrap(), true);
        assert_eq!(r.get("ok").unwrap(), true);
    }

    #[test]
    fn handled_ids_contains_core_set() {
        let ids = BridgeDispatcher::<MockHostBridge>::handled_ids();
        assert!(ids.contains(&"scene.create_entity"));
        assert!(ids.contains(&"component.add"));
        assert!(ids.contains(&"runtime.enter_play"));
        assert!(ids.contains(&"editor.undo"));
    }
}
