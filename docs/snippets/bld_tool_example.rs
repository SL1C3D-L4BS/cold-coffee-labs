// snippets/bld_tool_example.rs — Greywater_Engine · BLD crate
//
// The canonical BLD tool shape. `#[bld_tool]` generates the MCP schema,
// JSON decoder, dispatch handler, and registry entry at compile time —
// no hand-written JSON, no libclang pass, no external codegen tool.
//
// Tools live inside the `bld-tools` crate (for engine-agnostic ones)
// or inside the specific `bld-*` sub-crate that owns the domain.
//
// The pattern:
//
//   - Tools are free functions taking typed args, returning `Result<T, E>`.
//   - The proc macro derives the MCP schema from the signature.
//   - Mutating tools must declare a `tier = "mutate"` or `tier = "execute"`;
//     `bld-governance` enforces approval at dispatch time.
//   - Callbacks into the C++ editor go through `bld_ffi::editor_callback(...)`
//     — never touch C++ state directly.
//
// Run `cargo build -p bld-tools` to regenerate the registry after adding
// or changing a tool.

use bld_ffi::editor_callback;
use bld_tools::{bld_tool, ToolError, ToolResult};
use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize)]
pub struct EntityHandle(pub u64);

#[derive(Deserialize, Serialize)]
pub struct CreateEntityArgs {
    /// Human-readable name for the new entity. Used in the editor hierarchy.
    pub name: String,
    /// Optional parent entity. When set, the new entity inherits the parent's
    /// transform space.
    pub parent: Option<EntityHandle>,
}

/// Create a new entity in the active scene.
///
/// Tier: `mutate` — requires human approval per session (or per call under
/// strict governance mode). The action is committed via the editor command
/// bus so Ctrl+Z reverts it atomically.
#[bld_tool(
    namespace = "scene",
    name = "create_entity",
    tier = "mutate",
    summary = "Create a new entity in the active scene."
)]
pub fn create_entity(args: CreateEntityArgs) -> ToolResult<EntityHandle> {
    // Submit the command through the C-ABI callback. The editor enqueues
    // it on the command stack; the returned handle is guaranteed valid
    // only after the command executes.
    let handle = editor_callback::scene_create_entity(&args.name, args.parent.as_ref())
        .map_err(ToolError::editor)?;

    Ok(EntityHandle(handle))
}

// Tests use the in-memory editor stub so CI can exercise the tool
// registry without a running editor process.
#[cfg(test)]
mod tests {
    use super::*;
    use bld_ffi::test_stub as stub;

    #[test]
    fn create_entity_returns_valid_handle() {
        stub::reset();
        let handle = create_entity(CreateEntityArgs {
            name: "TestEntity".into(),
            parent: None,
        })
        .expect("stub should succeed");
        assert_ne!(handle.0, 0);
    }
}
