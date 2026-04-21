#pragma once
// editor/bld_api/editor_bld_api.hpp
// Stable C-ABI exported by gw_editor for BLD (Phase 9) to drive.
// Spec ref: Phase 7 §13.
//
// RULES:
//   - Never remove or reorder existing entries — only add at the tail.
//   - Version the surface with gw_editor_version().
//   - All calls are thread-safe; mutations are queued for main-thread execution.
//   - The main thread drains the queue at the start of each frame tick.

#include <cstdint>

#ifdef _WIN32
#  define GW_EDITOR_API extern "C" __declspec(dllexport)
#else
#  define GW_EDITOR_API extern "C" __attribute__((visibility("default")))
#endif

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
// Internal setup — called by EditorApplication on startup (same process,
// not exported to external callers).
// ---------------------------------------------------------------------------
namespace gw::editor { class SelectionContext; }
namespace gw::core   { class CommandStack; }

namespace gw::editor::bld_api {

    struct EditorGlobals {
        gw::editor::SelectionContext* selection  = nullptr;
        gw::core::CommandStack*       cmd_stack  = nullptr;
        void*                         world      = nullptr;   // gw::ecs::World* Phase 8
    };

    // Defined in editor_bld_api.cpp; written once by EditorApplication.
    extern EditorGlobals g_globals;

}  // namespace gw::editor::bld_api
