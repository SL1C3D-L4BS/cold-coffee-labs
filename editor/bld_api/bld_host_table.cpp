// editor/bld_api/bld_host_table.cpp
// Installs Surface P BldHostCallbacks (ADR-0007) so in-process BLD can call
// gw_editor_* and run_command. Phase 18-B adds run_command for sequencer tools.

#include "bld/include/bld_register.h"
#include "editor/bld_api/editor_bld_api.hpp"

extern "C" {

static uint32_t cb_surface_p_version(void) { return BLD_SURFACE_P_VERSION; }

static void cb_log(uint32_t level, const char* message_utf8) {
    gw_editor_log(level, message_utf8);
}

static uint64_t cb_get_primary_selection(void) { return gw_editor_get_primary_selection(); }

static uint32_t cb_get_selection_count(void) { return gw_editor_get_selection_count(); }

static void cb_set_selection(const uint64_t* handles, const uint32_t count) {
    gw_editor_set_selection(handles, count);
}

static uint64_t cb_create_entity(const char* name) { return gw_editor_create_entity(name); }

static bool cb_destroy_entity(const uint64_t handle) { return gw_editor_destroy_entity(handle); }

static bool cb_set_field(const uint64_t e, const char* path, const char* val) {
    return gw_editor_set_field(e, path, val);
}

static const char* cb_get_field(const uint64_t e, const char* path) { return gw_editor_get_field(e, path); }

static void cb_free_string(const char* s) { gw_free_string(s); }

static bool cb_undo(void) { return gw_editor_undo(); }

static bool cb_redo(void) { return gw_editor_redo(); }

static bool cb_save_scene(const char* path) { return gw_editor_save_scene(path); }

static bool cb_load_scene(const char* path) { return gw_editor_load_scene(path); }

static bool cb_enter_play(void) { return gw_editor_enter_play(); }

static bool cb_stop(void) { return gw_editor_stop(); }

static void cb_enumerate_components(BldComponentEnumFn /*cb*/, void* /*user*/) {}

static uint64_t cb_run_command(const uint32_t opcode, const void* payload, const uint32_t bytes) {
    return gw::editor::bld_api::dispatch_run_command(opcode, payload, bytes);
}

}  // extern "C"

static BldHostCallbacks g_bld_host_callbacks{
    .surface_p_version   = &cb_surface_p_version,
    .log                 = &cb_log,
    .get_primary_selection = &cb_get_primary_selection,
    .get_selection_count   = &cb_get_selection_count,
    .set_selection         = &cb_set_selection,
    .create_entity         = &cb_create_entity,
    .destroy_entity        = &cb_destroy_entity,
    .set_field             = &cb_set_field,
    .get_field             = &cb_get_field,
    .free_string           = &cb_free_string,
    .undo                  = &cb_undo,
    .redo                  = &cb_redo,
    .save_scene            = &cb_save_scene,
    .load_scene            = &cb_load_scene,
    .enter_play            = &cb_enter_play,
    .stop                  = &cb_stop,
    .enumerate_components  = &cb_enumerate_components,
    .run_command           = &cb_run_command,
    .run_command_batch     = nullptr,
    .session_create        = nullptr,
    .session_destroy       = nullptr,
    .session_id            = nullptr,
    .session_request       = nullptr,
};

void gw::editor::bld_api::install_bld_host_callbacks() noexcept {
    bld_abi_install_host_callbacks(&g_bld_host_callbacks);
}
