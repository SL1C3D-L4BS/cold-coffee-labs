//! `bld-tools` — the runtime tool registry plus the user-facing re-export of
//! `#[bld_tool]` (ADR-0012).
//!
//! The registry is inventory-driven: every `#[bld_tool]` emits an
//! `inventory::submit!(&ToolDescriptor)` entry, and `registry::all()`
//! returns every descriptor linked into the process. No central list, no
//! hand-maintained file.
//!
//! Phase 9A lands the scaffolding: descriptor type, tier parser, registry
//! helper, inventory re-export. Later waves (9B, 9D) flesh out the tool
//! bodies. The `macros` feature toggles the proc-macro re-export; only
//! downstream crates (`bld-agent`) flip it on so tests here stay fast.

#![warn(missing_docs)]
#![deny(clippy::unwrap_used, clippy::expect_used)]

// `#[bld_tool]` expands to `::bld_tools::...`; for tools defined inside this
// crate (see `tools::seq_tools`), alias the crate so the macro resolves.
#[cfg(feature = "seq-tools")]
extern crate self as bld_tools;

pub use inventory;

use bld_governance::tier::Tier;

/// Descriptor emitted by the `#[bld_tool]` macro.
///
/// `id` is a dotted, lower-snake-case string (`"scene.create_entity"`).
/// `tier` is the authorization class. `summary` is shown in the chat
/// transcript, the approval dialog, and tool search results.
#[derive(Debug, Clone, Copy)]
pub struct ToolDescriptor {
    /// Dotted tool id.
    pub id: &'static str,
    /// Authorization tier.
    pub tier: Tier,
    /// One-line summary (markdown-safe).
    pub summary: &'static str,
}

// Inventory collector for descriptors — every `#[bld_tool]` submits into this.
inventory::collect!(&'static ToolDescriptor);

/// A runtime dispatch entry emitted by `#[bld_tool]` for tools written in
/// pure Rust (ADR-0012 §2.3). The registry distinguishes between
/// *descriptor-only* tools (stubbed in `taxonomy.rs` for wire-surface
/// advertisement) and *dispatchable* tools (real bodies emitted by the
/// macro).
///
/// The `dispatch` function receives the input JSON and must return the
/// output JSON or an error string. It is sync on purpose: async tool
/// bodies are supported by having the macro wrap the body in a
/// `tokio::runtime::Handle::block_on` — but for the 9B shim the
/// synchronous surface is enough.
#[derive(Debug, Clone, Copy)]
pub struct ToolDispatchEntry {
    /// Must match a registered `ToolDescriptor::id`.
    pub id: &'static str,
    /// Sync dispatcher. Returns JSON result or error message.
    pub dispatch: fn(&serde_json::Value) -> Result<serde_json::Value, String>,
}

inventory::collect!(&'static ToolDispatchEntry);

/// Convert the macro-emitted tier string into a `Tier` at compile time.
///
/// The proc-macro already validated the literal is one of `Read`,
/// `Mutate`, `Execute`. If an unexpected value survives somehow we fall
/// back to the most restrictive tier (`Execute`) so the user always gets
/// elicitation rather than silent escalation.
#[must_use]
pub const fn tier_from_str(s: &'static str) -> Tier {
    // `==` on strings isn't const yet; compare byte-by-byte.
    let b = s.as_bytes();
    const READ: &[u8] = b"Read";
    const MUTATE: &[u8] = b"Mutate";
    if eq(b, READ) {
        Tier::Read
    } else if eq(b, MUTATE) {
        Tier::Mutate
    } else {
        Tier::Execute
    }
}

const fn eq(a: &[u8], b: &[u8]) -> bool {
    if a.len() != b.len() {
        return false;
    }
    let mut i = 0;
    while i < a.len() {
        if a[i] != b[i] {
            return false;
        }
        i += 1;
    }
    true
}

/// Runtime tool registry helpers.
pub mod registry {
    use super::{ToolDescriptor, ToolDispatchEntry};

    /// Iterate every `#[bld_tool]` descriptor linked into the process.
    pub fn all() -> impl Iterator<Item = &'static ToolDescriptor> {
        inventory::iter::<&'static ToolDescriptor>
            .into_iter()
            .copied()
    }

    /// Count of registered descriptors.
    pub fn count() -> usize {
        all().count()
    }

    /// Look up by id. O(n) — the registry is tiny (~80 tools at Phase 9
    /// end per ADR-0012) so this is fine.
    pub fn find(id: &str) -> Option<&'static ToolDescriptor> {
        all().find(|d| d.id == id)
    }

    /// Iterate every dispatchable entry (ADR-0012 §2.3).
    pub fn dispatch_entries() -> impl Iterator<Item = &'static ToolDispatchEntry> {
        inventory::iter::<&'static ToolDispatchEntry>
            .into_iter()
            .copied()
    }

    /// Look up a dispatch entry by id.
    pub fn find_dispatch(id: &str) -> Option<&'static ToolDispatchEntry> {
        dispatch_entries().find(|e| e.id == id)
    }

    /// Count of dispatchable tools.
    pub fn dispatch_count() -> usize {
        dispatch_entries().count()
    }
}

/// Tool errors surfaced to the agent.
#[derive(Debug, thiserror::Error)]
pub enum ToolError {
    /// Input JSON failed schema validation.
    #[error("bad input: {0}")]
    BadInput(String),
    /// The host reported a failure.
    #[error("host: {0}")]
    Host(String),
    /// Tool not registered.
    #[error("unknown tool: {0}")]
    Unknown(String),
    /// Tool is not yet implemented (scaffolded stub).
    #[error("tool not yet implemented: {0}")]
    Unimplemented(String),
}

#[cfg(feature = "macros")]
pub use bld_tools_macros::{bld_component_tools, bld_tool};

/// Sequencer and host-bridge tools (Phase 18-B). Enable with `--features seq-tools`.
#[cfg(feature = "seq-tools")]
pub mod tools;

// The Phase 9 tool taxonomy — 78 additional stubs bring the registry to 79.
pub mod taxonomy;

// Sacrilege copilot tools (Part B §12.6, ADR-0109). Seven descriptor-only
// stubs; bodies land in Phase 22/23/26/28.
pub mod sacrilege_tools;

/// Autogenerate `component.<Type>.{get|set_*|reset}` tool descriptors from
/// the editor's reflection manifest (ADR-0012 §2.5).
pub mod component_autogen;

/// Output-coercion helpers used by the `#[bld_tool]` macro-generated
/// dispatch shims.
///
/// The macro emits one of two shim variants, chosen at parse time:
///
/// - `serialize_ok(val)` for tools that return `T: Serialize` directly.
/// - `serialize_result(res)` for tools that return `Result<T, E>`.
///
/// Both collapse to `Result<serde_json::Value, String>`.
pub mod dispatch_shim {
    use serde::Serialize;

    /// Serialize a value as the tool's success output.
    pub fn serialize_ok<T: Serialize>(val: T) -> Result<serde_json::Value, String> {
        serde_json::to_value(val).map_err(|e| format!("serialize: {e}"))
    }

    /// Serialize a `Result`-returning tool body.
    pub fn serialize_result<T: Serialize, E: core::fmt::Display>(
        res: Result<T, E>,
    ) -> Result<serde_json::Value, String> {
        match res {
            Ok(v) => serde_json::to_value(v).map_err(|e| format!("serialize: {e}")),
            Err(e) => Err(e.to_string()),
        }
    }
}

// -----------------------------------------------------------------------------
// Built-in demo tool so the registry has at least one entry for smoke tests.
// This is a project.list_scenes stub. Wave 9C replaces it with the real body.
// -----------------------------------------------------------------------------

/// Tier of the scaffolded `project.list_scenes` tool, exposed for tests.
pub const LIST_SCENES_TIER: Tier = Tier::Read;

#[doc(hidden)]
pub static PROJECT_LIST_SCENES: ToolDescriptor = ToolDescriptor {
    id: "project.list_scenes",
    tier: LIST_SCENES_TIER,
    summary: "Enumerate every .scene file under the project root.",
};

inventory::submit!(&PROJECT_LIST_SCENES);

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn registry_has_seed_tool() {
        let all: Vec<_> = registry::all().collect();
        assert!(
            all.iter().any(|d| d.id == "project.list_scenes"),
            "project.list_scenes must be registered"
        );
    }

    #[test]
    fn tier_from_str_is_total() {
        assert_eq!(tier_from_str("Read"), Tier::Read);
        assert_eq!(tier_from_str("Mutate"), Tier::Mutate);
        assert_eq!(tier_from_str("Execute"), Tier::Execute);
        // Fall-back is Execute for safety.
        assert_eq!(tier_from_str("wat"), Tier::Execute);
    }

    #[test]
    fn find_by_id_works() {
        assert!(registry::find("project.list_scenes").is_some());
        assert!(registry::find("nonexistent.tool").is_none());
    }

    #[test]
    fn count_is_nonzero() {
        assert!(registry::count() >= 1);
    }
}
