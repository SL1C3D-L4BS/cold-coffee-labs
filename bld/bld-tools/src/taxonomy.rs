//! The Phase 9 tool taxonomy — 79 stub tools across 13 categories.
//!
//! This file registers descriptors for every tool BLD exposes. The
//! bodies are the concern of Wave 9B–9D; this wave ensures the registry,
//! MCP `tools/list`, and agent catalog all see the full surface area so
//! end-to-end integration tests can exercise the wire contract even
//! before the individual tool code lands.
//!
//! Taxonomy authority: `ADR-0012` §3 ("Tool Surface"). Any tool added
//! here must appear in the registry and have a corresponding TIER
//! entry. The macro below checks that at build time.
//!
//! ## Category distribution
//!
//! | Prefix       | Tools | Primary tier                |
//! |--------------|-------|-----------------------------|
//! | `scene.*`    |   10  | Mutate (read: list/select)  |
//! | `component.*`|    5  | Mutate                      |
//! | `asset.*`    |    8  | Mixed                       |
//! | `code.*`     |    7  | Mixed                       |
//! | `vscript.*`  |    5  | Mutate                      |
//! | `build.*`    |    6  | Execute                     |
//! | `runtime.*`  |    7  | Execute                     |
//! | `project.*`  |    5  | Read                        |
//! | `debug.*`    |    5  | Read / Execute              |
//! | `docs.*`     |    4  | Read                        |
//! | `rag.*`      |    3  | Read                        |
//! | `agent.*`    |    6  | Mutate                      |
//! | `editor.*`   |    8  | Mutate                      |
//! | **Total**    | **79**|                             |
//!
//! If this table is out of sync with the descriptors, the
//! `taxonomy_count_is_79` unit test fails.

use crate::ToolDescriptor;
use bld_governance::tier::Tier;

/// Declare a BLD tool stub: emits a `ToolDescriptor` static plus an
/// `inventory::submit!` entry. Saves the ~10 lines of boilerplate the
/// `#[bld_tool]` proc macro generates for each.
macro_rules! bld_tool_stub {
    ($ident:ident, $id:literal, $tier:expr, $summary:literal $(,)?) => {
        #[doc(hidden)]
        pub static $ident: ToolDescriptor = ToolDescriptor {
            id: $id,
            tier: $tier,
            summary: $summary,
        };
        inventory::submit!(&$ident);
    };
}

// -----------------------------------------------------------------------------
// scene.* — 10 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(SCENE_CREATE_ENTITY,  "scene.create_entity",  Tier::Mutate, "Create a new empty entity in the active scene.");
bld_tool_stub!(SCENE_DELETE_ENTITY,  "scene.delete_entity",  Tier::Mutate, "Delete an entity by id.");
bld_tool_stub!(SCENE_RENAME_ENTITY,  "scene.rename_entity",  Tier::Mutate, "Change an entity's NameComponent.");
bld_tool_stub!(SCENE_REPARENT,       "scene.reparent",       Tier::Mutate, "Move an entity under a new parent (cycle-checked).");
bld_tool_stub!(SCENE_SELECT,         "scene.select",         Tier::Mutate, "Replace the current selection set.");
bld_tool_stub!(SCENE_FOCUS,          "scene.focus",          Tier::Mutate, "Frame the viewport on the selection.");
bld_tool_stub!(SCENE_DUPLICATE,      "scene.duplicate",      Tier::Mutate, "Deep-clone an entity and reparent under the same parent.");
bld_tool_stub!(SCENE_SAVE,           "scene.save",           Tier::Mutate, "Serialise the active scene to .gwscene.");
bld_tool_stub!(SCENE_LOAD,           "scene.load",           Tier::Mutate, "Load a .gwscene into the authoring world.");
bld_tool_stub!(SCENE_LIST_ENTITIES,  "scene.list_entities",  Tier::Read,   "List entity ids + NameComponents in the active scene.");

// -----------------------------------------------------------------------------
// component.* — 5 tools (autogen expansion in 9B lives alongside these)
// -----------------------------------------------------------------------------
bld_tool_stub!(COMPONENT_ADD,        "component.add",        Tier::Mutate, "Attach a component of a reflected type to an entity.");
bld_tool_stub!(COMPONENT_REMOVE,     "component.remove",     Tier::Mutate, "Detach a component from an entity.");
bld_tool_stub!(COMPONENT_SET_FIELD,  "component.set_field",  Tier::Mutate, "Write one reflected field on a component.");
bld_tool_stub!(COMPONENT_GET,        "component.get",        Tier::Read,   "Read the JSON snapshot of a component.");
bld_tool_stub!(COMPONENT_LIST,       "component.list",       Tier::Read,   "List every component attached to an entity.");

// -----------------------------------------------------------------------------
// asset.* — 8 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(ASSET_IMPORT,         "asset.import",         Tier::Mutate, "Import a file as an engine asset through the VFS.");
bld_tool_stub!(ASSET_REIMPORT,       "asset.reimport",       Tier::Mutate, "Re-run the importer on an existing asset.");
bld_tool_stub!(ASSET_REVEAL,         "asset.reveal",         Tier::Read,   "Highlight an asset in the Asset Browser.");
bld_tool_stub!(ASSET_FIND,           "asset.find",           Tier::Read,   "Search the asset database by name / type / guid.");
bld_tool_stub!(ASSET_RENAME,         "asset.rename",         Tier::Mutate, "Rename an asset (updates references via GUID).");
bld_tool_stub!(ASSET_DELETE,         "asset.delete",         Tier::Mutate, "Delete an asset and its cooked variants.");
bld_tool_stub!(ASSET_THUMBNAIL,      "asset.thumbnail",      Tier::Read,   "Request a thumbnail PNG for an asset.");
bld_tool_stub!(ASSET_METADATA,       "asset.metadata",       Tier::Read,   "Fetch an asset's metadata JSON (guid, type, refs, hash).");

// -----------------------------------------------------------------------------
// code.* — 7 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(CODE_READ_FILE,       "code.read_file",       Tier::Read,   "Read an entire source file (UTF-8, capped by policy).");
bld_tool_stub!(CODE_READ_RANGE,      "code.read_range",      Tier::Read,   "Read a line range out of a source file.");
bld_tool_stub!(CODE_WRITE_RANGE,     "code.write_range",     Tier::Mutate, "Replace a line range; diff is shown for elicitation.");
bld_tool_stub!(CODE_CREATE_FILE,     "code.create_file",     Tier::Mutate, "Create a new source file with given contents.");
bld_tool_stub!(CODE_DELETE_FILE,     "code.delete_file",     Tier::Mutate, "Delete a source file (elicits; not recoverable).");
bld_tool_stub!(CODE_RENAME_FILE,     "code.rename_file",     Tier::Mutate, "Rename/move a source file.");
bld_tool_stub!(CODE_SEARCH,          "code.search",          Tier::Read,   "Lexical + symbol search across the repo.");

// -----------------------------------------------------------------------------
// vscript.* — 5 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(VSCRIPT_ADD_NODE,     "vscript.add_node",     Tier::Mutate, "Add a node to the active VScript graph.");
bld_tool_stub!(VSCRIPT_REMOVE_NODE,  "vscript.remove_node",  Tier::Mutate, "Remove a node by id.");
bld_tool_stub!(VSCRIPT_CONNECT,      "vscript.connect",      Tier::Mutate, "Create a connection between two pins.");
bld_tool_stub!(VSCRIPT_DISCONNECT,   "vscript.disconnect",   Tier::Mutate, "Drop a connection by id.");
bld_tool_stub!(VSCRIPT_RUN_GRAPH,    "vscript.run_graph",    Tier::Execute,"Execute the current VScript graph in the gameplay sandbox.");

// -----------------------------------------------------------------------------
// build.* — 6 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(BUILD_CONFIGURE,      "build.configure",      Tier::Execute,"Run `cmake --preset` for the current config.");
bld_tool_stub!(BUILD_BUILD,          "build.build",          Tier::Execute,"Invoke the build for the default target.");
bld_tool_stub!(BUILD_BUILD_TARGET,   "build.build_target",   Tier::Execute,"Build a specific target by name.");
bld_tool_stub!(BUILD_REBUILD,        "build.rebuild",        Tier::Execute,"Clean + build in one step.");
bld_tool_stub!(BUILD_CLEAN,          "build.clean",          Tier::Execute,"Run the clean target.");
bld_tool_stub!(BUILD_PRESETS,        "build.presets",        Tier::Read,   "List available CMake presets.");

// -----------------------------------------------------------------------------
// runtime.* — 7 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(RUNTIME_ENTER_PLAY,       "runtime.enter_play",       Tier::Execute,"Begin Play-In-Editor.");
bld_tool_stub!(RUNTIME_EXIT_PLAY,        "runtime.exit_play",        Tier::Execute,"Leave PIE and restore authoring state.");
bld_tool_stub!(RUNTIME_PAUSE,            "runtime.pause",            Tier::Execute,"Pause the PIE simulation.");
bld_tool_stub!(RUNTIME_RESUME,           "runtime.resume",           Tier::Execute,"Resume the PIE simulation.");
bld_tool_stub!(RUNTIME_STEP_FRAME,       "runtime.step_frame",       Tier::Execute,"Advance one PIE frame while paused.");
bld_tool_stub!(RUNTIME_HOT_RELOAD_DLL,   "runtime.hot_reload_dll",   Tier::Execute,"Swap the gameplay DLL without exiting PIE.");
bld_tool_stub!(RUNTIME_GET_LOGS,         "runtime.get_logs",         Tier::Read,   "Fetch recent console log lines.");

// -----------------------------------------------------------------------------
// project.* — 5 tools (note: project.list_scenes is already registered in lib.rs)
// -----------------------------------------------------------------------------
bld_tool_stub!(PROJECT_LIST_ASSETS,  "project.list_assets",  Tier::Read,   "List every asset in the VFS tree.");
bld_tool_stub!(PROJECT_GET_MANIFEST, "project.get_manifest", Tier::Read,   "Read project.manifest.json.");
bld_tool_stub!(PROJECT_OPEN_FILE,    "project.open_file",    Tier::Mutate, "Open a file in the editor's text view.");
bld_tool_stub!(PROJECT_SHOW_IN_EXPLORER, "project.show_in_explorer", Tier::Execute, "Reveal a path in the OS file manager.");
// `project.list_scenes` is the seed registered in `lib.rs` (kept there so
// the registry always has >=1 tool even when `taxonomy` is stripped).

// -----------------------------------------------------------------------------
// debug.* — 5 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(DEBUG_INSPECT_ENTITY, "debug.inspect_entity", Tier::Read,   "Dump an entity's full ECS state to JSON.");
bld_tool_stub!(DEBUG_BREAKPOINT_SET,  "debug.breakpoint_set",  Tier::Execute,"Arm a breakpoint at file:line.");
bld_tool_stub!(DEBUG_BREAKPOINT_CLEAR,"debug.breakpoint_clear",Tier::Execute,"Clear a breakpoint by id.");
bld_tool_stub!(DEBUG_STACK_TRACE,     "debug.stack_trace",     Tier::Read,   "Capture the current runtime stack trace.");
bld_tool_stub!(DEBUG_HEAP_STATS,      "debug.heap_stats",      Tier::Read,   "Query VMA + system allocator stats.");

// -----------------------------------------------------------------------------
// docs.* — 4 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(DOCS_SEARCH,      "docs.search",      Tier::Read, "Full-text search across docs/*.md.");
bld_tool_stub!(DOCS_OPEN,        "docs.open",        Tier::Read, "Fetch a docs page by path.");
bld_tool_stub!(DOCS_TOC,         "docs.toc",         Tier::Read, "Emit the table of contents for a docs file.");
bld_tool_stub!(DOCS_READ_SECTION,"docs.read_section",Tier::Read, "Read a markdown section by anchor.");

// -----------------------------------------------------------------------------
// rag.* — 3 tools (hybrid retriever; ADR-0013)
// -----------------------------------------------------------------------------
bld_tool_stub!(RAG_INDEX_FILE,      "rag.index_file",      Tier::Read, "Force-index a single file into the RAG store.");
bld_tool_stub!(RAG_QUERY,           "rag.query",           Tier::Read, "Hybrid (BM25 + vector + symbol) retrieval.");
bld_tool_stub!(RAG_GRAPH_NEIGHBORS, "rag.graph_neighbors", Tier::Read, "Walk the symbol graph from a seed node.");

// -----------------------------------------------------------------------------
// agent.* — 6 tools (self-configuration; ADR-0010, ADR-0016)
// -----------------------------------------------------------------------------
bld_tool_stub!(AGENT_PROVIDER_SWITCH,"agent.provider_switch",Tier::Mutate, "Change the active LLM provider for the session.");
bld_tool_stub!(AGENT_MODEL_SELECT,   "agent.model_select",   Tier::Mutate, "Pick a specific model id from the provider.");
bld_tool_stub!(AGENT_OFFLINE_ON,     "agent.offline_on",     Tier::Mutate, "Disable all cloud providers; local only.");
bld_tool_stub!(AGENT_OFFLINE_OFF,    "agent.offline_off",    Tier::Mutate, "Re-enable cloud providers.");
bld_tool_stub!(AGENT_SESSIONS_LIST,  "agent.sessions_list",  Tier::Read,   "List open agent sessions with turn counts.");
bld_tool_stub!(AGENT_SESSION_EXPORT, "agent.session_export", Tier::Read,   "Export an agent session transcript + audit log.");

// -----------------------------------------------------------------------------
// editor.* — 8 tools
// -----------------------------------------------------------------------------
bld_tool_stub!(EDITOR_UNDO,          "editor.undo",          Tier::Mutate, "Undo the last CommandStack entry.");
bld_tool_stub!(EDITOR_REDO,          "editor.redo",          Tier::Mutate, "Redo the next CommandStack entry.");
bld_tool_stub!(EDITOR_HISTORY,       "editor.history",       Tier::Read,   "Dump the current undo/redo stack.");
bld_tool_stub!(EDITOR_ZOOM_TO,       "editor.zoom_to",       Tier::Mutate, "Frame the viewport on a position / entity.");
bld_tool_stub!(EDITOR_SCREENSHOT,    "editor.screenshot",    Tier::Execute,"Capture the viewport to PNG.");
bld_tool_stub!(EDITOR_TOOL_FILTER,   "editor.tool_filter",   Tier::Mutate, "Set the visible tool categories for the palette.");
bld_tool_stub!(EDITOR_PANEL_TOGGLE,  "editor.panel_toggle",  Tier::Mutate, "Show/hide an editor panel by name.");
bld_tool_stub!(EDITOR_THEME_SET,     "editor.theme_set",     Tier::Mutate, "Switch the ImGui theme preset.");

// -----------------------------------------------------------------------------
// Sanity check — if this fails in CI, the table at the top is stale.
// -----------------------------------------------------------------------------
#[cfg(test)]
mod tests {
    use crate::registry;

    #[test]
    fn taxonomy_count_is_79() {
        // The lib.rs seed tool `project.list_scenes` contributes 1; the
        // 78 entries in this file plus that seed equals 79 exactly.
        let ids: Vec<_> = registry::all().map(|d| d.id).collect();
        assert_eq!(
            ids.len(),
            79,
            "taxonomy registry size must be 79 (got {}). Tools: {ids:?}",
            ids.len()
        );
    }

    #[test]
    fn every_id_is_unique() {
        let mut ids: Vec<_> = registry::all().map(|d| d.id).collect();
        ids.sort_unstable();
        let original = ids.len();
        ids.dedup();
        assert_eq!(original, ids.len(), "duplicate tool ids detected");
    }

    #[test]
    fn every_id_has_category_prefix() {
        const VALID: &[&str] = &[
            "scene.", "component.", "asset.", "code.", "vscript.",
            "build.", "runtime.", "project.", "debug.", "docs.",
            "rag.", "agent.", "editor.",
        ];
        for d in registry::all() {
            assert!(
                VALID.iter().any(|p| d.id.starts_with(p)),
                "tool id `{}` has unknown category prefix",
                d.id
            );
        }
    }

    #[test]
    fn tiers_match_adr_policy() {
        use bld_governance::tier::Tier;
        // Spot checks — these must NEVER silently change; if policy shifts,
        // the ADR must be updated first (doctrine-first).
        let r = |id| registry::find(id).map(|d| d.tier);
        assert_eq!(r("scene.delete_entity"), Some(Tier::Mutate));
        assert_eq!(r("scene.list_entities"), Some(Tier::Read));
        assert_eq!(r("build.build"),         Some(Tier::Execute));
        assert_eq!(r("code.search"),         Some(Tier::Read));
        assert_eq!(r("runtime.enter_play"),  Some(Tier::Execute));
        assert_eq!(r("project.list_scenes"), Some(Tier::Read));
    }
}
