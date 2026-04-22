#pragma once
// editor/bld_api/editor_bld_api.hpp
// Surface P — Stable C-ABI exported by gw_editor for BLD (Phase 9) to drive.
// Frozen at BLD_ABI_VERSION = 1. See docs/10_APPENDIX_ADRS_AND_REFERENCES.md.
//
// RULES (lifted to ADR-0007 status; the comment here is a quick-ref):
//   - Never remove or reorder existing entries — only add at the tail.
//   - Every item tagged with the BLD_ABI_VERSION that introduced it.
//   - All calls are thread-safe; mutations queued for the main thread.
//   - The main thread drains the queue at the start of each frame tick.
//   - std::string_view never crosses this boundary (NN #8).
//   - Input strings are caller-owned, valid only for the call duration.
//   - Output strings are caller-owned, free via gw_free_string.

#include <cstdint>

#ifdef _WIN32
#  define GW_EDITOR_API extern "C" __declspec(dllexport)
#else
#  define GW_EDITOR_API extern "C" __attribute__((visibility("default")))
#endif

// Editor build string (e.g. "gw_editor/0.7.0"). Informational; BLD uses
// bld_abi_version() from Surface H for negotiation.
GW_EDITOR_API const char* gw_editor_version();

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------
GW_EDITOR_API uint64_t gw_editor_get_primary_selection();
GW_EDITOR_API uint32_t gw_editor_get_selection_count();
GW_EDITOR_API void     gw_editor_set_selection(const uint64_t* handles,
                                                uint32_t        count);

// ---------------------------------------------------------------------------
// Entity management
// ---------------------------------------------------------------------------
GW_EDITOR_API uint64_t gw_editor_create_entity(const char* name);
GW_EDITOR_API bool     gw_editor_destroy_entity(uint64_t handle);

// ---------------------------------------------------------------------------
// Component field access via reflected path "ComponentName.field_name".
// value_json is UTF-8 JSON-encoded. Caller owns input strings.
// gw_editor_get_field returns a heap string; free with gw_free_string().
// ---------------------------------------------------------------------------
GW_EDITOR_API bool        gw_editor_set_field(uint64_t    entity,
                                               const char* field_path,
                                               const char* value_json);
GW_EDITOR_API const char* gw_editor_get_field(uint64_t    entity,
                                               const char* field_path);
GW_EDITOR_API void        gw_free_string(const char* s);

// ---------------------------------------------------------------------------
// Command stack
// ---------------------------------------------------------------------------
GW_EDITOR_API bool        gw_editor_undo();
GW_EDITOR_API bool        gw_editor_redo();
GW_EDITOR_API const char* gw_editor_command_stack_summary(uint32_t max_entries);

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_save_scene(const char* path);
GW_EDITOR_API bool gw_editor_load_scene(const char* path);

// ---------------------------------------------------------------------------
// PIE
// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_enter_play();
GW_EDITOR_API bool gw_editor_stop();

// ---------------------------------------------------------------------------
// Registrar + tool/widget registration (ADR-0007 §2.4). Pulled in from the
// BLD-side header so the types are identical across both surfaces.
// ---------------------------------------------------------------------------
#include "bld/include/bld_register.h"

// Introduced in v1. Tool invocation callback — runs on the editor main
// thread. user_data is BLD-owned; editor never dereferences or frees it.
typedef void (*BldToolFn)(void* user_data);

// Introduced in v1. Inspector widget callback — called inside ImGui's
// component block (between BeginChild/EndChild). MUST NOT Begin/End its
// own ImGui window.
typedef void (*BldWidgetFn)(void* component_ptr, void* user_data);

// Register a named tool (ADR-0007 §2.4).
//   id          : stable reverse-DNS-style id, e.g. "org.brewedlogic.hello".
//   name        : display string for menus.
//   tooltip_utf8: hover text; may be empty "".
//   icon_name   : optional icon id; nullptr for none.
//   fn          : invoked when the tool runs (main thread).
// Returns non-zero tool id on success, 0 on failure (duplicate id, null
// registrar, etc.). `name`/`tooltip`/`icon_name` are copied into the editor;
// not retained after the call.
GW_EDITOR_API uint32_t bld_registrar_register_tool(BldRegistrar* reg,
                                                    const char*    id,
                                                    const char*    name,
                                                    const char*    tooltip_utf8,
                                                    const char*    icon_name,
                                                    BldToolFn      fn,
                                                    void*          user_data);

// Register a custom inspector widget for a component hash (ADR-0007 §2.4).
// Returns non-zero widget id on success, 0 on failure.
GW_EDITOR_API uint32_t bld_registrar_register_widget(BldRegistrar* reg,
                                                      uint64_t     component_hash,
                                                      BldWidgetFn  draw_fn,
                                                      void*        user_data);

// Invoke a registered tool by id (ADR-0007 §2.4). Returns false if id is
// unknown. The tool's own error propagation is BLD's responsibility —
// typically via gw_editor_log_error.
GW_EDITOR_API bool gw_editor_run_tool(const char* tool_id);

// ---------------------------------------------------------------------------
// Logging surface for BLD → editor console panel (ADR-0007 §2.4).
// level: 0 = info, 1 = warn, 2 = error. Higher levels clamp to 2.
// ---------------------------------------------------------------------------
GW_EDITOR_API void gw_editor_log(uint32_t level, const char* message_utf8);
GW_EDITOR_API void gw_editor_log_error(const char* message_utf8);

// ---------------------------------------------------------------------------
// Internal setup — called by EditorApplication on startup (same process,
// not exported to external callers).
// ---------------------------------------------------------------------------
namespace gw::editor       { class SelectionContext; }
namespace gw::editor::undo { class CommandStack; }
namespace gw::ecs          { class World; }

namespace gw::editor::bld_api {

    // Log sink hook — editor wires this so gw_editor_log feeds the console
    // panel. Tests and headless runs can install their own sink.
    using LogSinkFn = void (*)(uint32_t level, const char* message);

    struct EditorGlobals {
        gw::editor::SelectionContext*  selection  = nullptr;
        gw::editor::undo::CommandStack* cmd_stack = nullptr;
        gw::ecs::World*                world      = nullptr;
        LogSinkFn                      log_sink   = nullptr;
    };

    // Defined in editor_bld_api.cpp; written once by EditorApplication.
    extern EditorGlobals g_globals;

    // Exposed for tests — constructs a fresh BldRegistrar the host owns.
    // The registrar lives until release_registrar() is called.
    [[nodiscard]] BldRegistrar* acquire_registrar();
    void                        release_registrar(BldRegistrar* reg);

    // Lookup for gw_editor_run_tool — tests want to poke this without
    // exporting more symbols.
    [[nodiscard]] bool invoke_tool(const char* tool_id);

}  // namespace gw::editor::bld_api
