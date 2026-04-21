// editor/bld_api/editor_bld_api.cpp
// Thread-safe C-ABI surface for BLD (Phase 9).
// Spec ref: Phase 7 §13.
#include "editor_bld_api.hpp"

// Full type definitions required — the header only forward-declares these.
#include "editor/selection/selection_context.hpp"
#include "editor/scene/components.hpp"
#include "editor/undo/command_stack.hpp"
#include "engine/ecs/world.hpp"

#include <cstdlib>
#include <cstring>

namespace gw::editor::bld_api {
    EditorGlobals g_globals;   // definition
}

using namespace gw::editor::bld_api;

// ---------------------------------------------------------------------------
GW_EDITOR_API const char* gw_editor_version() {
    return "gw_editor/0.7.0";
}

// ---------------------------------------------------------------------------
GW_EDITOR_API uint64_t gw_editor_get_primary_selection() {
    if (!g_globals.selection) return 0u;
    return g_globals.selection->primary().bits;
}

GW_EDITOR_API uint32_t gw_editor_get_selection_count() {
    if (!g_globals.selection) return 0u;
    return g_globals.selection->count();
}

GW_EDITOR_API void gw_editor_set_selection(const uint64_t* handles,
                                            uint32_t        count) {
    if (!g_globals.selection || !handles || count == 0) return;
    g_globals.selection->clear();
    for (uint32_t i = 0; i < count; ++i)
        g_globals.selection->toggle(gw::ecs::Entity{handles[i]});
}

// ---------------------------------------------------------------------------
GW_EDITOR_API uint64_t gw_editor_create_entity(const char* name) {
    if (!g_globals.world) return 0u;
    const auto e = g_globals.world->create_entity();
    g_globals.world->add_component(e,
        gw::editor::scene::NameComponent{name ? name : "Entity"});
    g_globals.world->add_component(e, gw::editor::scene::TransformComponent{});
    return e.bits;
}

GW_EDITOR_API bool gw_editor_destroy_entity(uint64_t handle) {
    if (!g_globals.world) return false;
    const gw::ecs::Entity e{handle};
    if (!g_globals.world->is_alive(e)) return false;
    g_globals.world->destroy_entity(e);
    return true;
}

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_set_field(uint64_t    /*entity*/,
                                        const char* /*field_path*/,
                                        const char* /*value_json*/) {
    return false;  // TODO(Phase 8)
}

GW_EDITOR_API const char* gw_editor_get_field(uint64_t    /*entity*/,
                                               const char* /*field_path*/) {
    char* s = static_cast<char*>(std::malloc(3));
    if (s) { s[0]='n'; s[1]='u'; s[2]='\0'; }
    return s;
}

GW_EDITOR_API void gw_free_string(const char* s) {
    std::free(const_cast<char*>(s)); // NOLINT
}

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_undo() {
    if (!g_globals.cmd_stack || !g_globals.cmd_stack->can_undo()) return false;
    g_globals.cmd_stack->undo();
    return true;
}

GW_EDITOR_API bool gw_editor_redo() {
    if (!g_globals.cmd_stack || !g_globals.cmd_stack->can_redo()) return false;
    g_globals.cmd_stack->redo();
    return true;
}

GW_EDITOR_API const char* gw_editor_command_stack_summary(uint32_t max_entries) {
    char tmp[4096] = {};
    if (g_globals.cmd_stack)
        g_globals.cmd_stack->summary(tmp, sizeof(tmp), max_entries);
    const auto len = std::strlen(tmp);
    char* s = static_cast<char*>(std::malloc(len + 1));
    if (s) std::memcpy(s, tmp, len + 1);
    return s;
}

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_save_scene(const char* /*path*/) { return false; }
GW_EDITOR_API bool gw_editor_load_scene(const char* /*path*/) { return false; }

// ---------------------------------------------------------------------------
GW_EDITOR_API bool gw_editor_enter_play() { return false; }
GW_EDITOR_API bool gw_editor_stop()       { return false; }
