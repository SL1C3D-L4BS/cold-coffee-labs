// editor/pie/gameplay_host.cpp
// Platform-specific dynamic-library loading isolated to this file.
// Spec ref: Phase 7 §12 — "LoadLibrary / dlopen goes in gameplay_host_platform.cpp
// with #ifdef _WIN32".  We keep one file here; macros guard platform code.
#include "gameplay_host.hpp"

#include <cassert>
#include <cstdio>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
static void* platform_load(const char* path) {
    return static_cast<void*>(LoadLibraryA(path));
}
static void platform_unload(void* h) { FreeLibrary(static_cast<HMODULE>(h)); }
static void* platform_sym(void* h, const char* sym) {
    return reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(h), sym));
}
static bool platform_copy_file(const char* src, const char* dst) {
    return CopyFileA(src, dst, FALSE) != 0;
}
static uint64_t platform_mtime(const char* path) {
    WIN32_FILE_ATTRIBUTE_DATA info{};
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &info)) return 0;
    ULARGE_INTEGER t{};
    t.LowPart  = info.ftLastWriteTime.dwLowDateTime;
    t.HighPart = info.ftLastWriteTime.dwHighDateTime;
    return t.QuadPart;
}
#else
#include <dlfcn.h>
#include <sys/stat.h>
static void* platform_load(const char* path) {
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}
static void platform_unload(void* h) { dlclose(h); }
static void* platform_sym(void* h, const char* sym) { return dlsym(h, sym); }
static bool platform_copy_file(const char* src, const char* dst) {
    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", src, dst);
    return system(cmd) == 0; // NOLINT — acceptable for editor-only PIE
}
static uint64_t platform_mtime(const char* path) {
    struct stat st{};
    if (stat(path, &st) != 0) return 0;
    return static_cast<uint64_t>(st.st_mtime);
}
#endif

namespace gw::editor {

GameplayHost::~GameplayHost() {
    if (lib_handle_) unload_dll();
}

bool GameplayHost::load_dll() {
    if (lib_source_path_.empty()) return false;

    // Copy to a temp path so the linker can overwrite the original.
    char temp[512];
#ifdef _WIN32
    std::snprintf(temp, sizeof(temp), "%s_hot_%03u.dll",
                  lib_source_path_.c_str(), hot_copy_counter_++);
#else
    std::snprintf(temp, sizeof(temp), "%s_hot_%03u.so",
                  lib_source_path_.c_str(), hot_copy_counter_++);
#endif
    lib_copy_path_ = temp;

    if (!platform_copy_file(lib_source_path_.c_str(), lib_copy_path_.c_str()))
        return false;

    lib_handle_ = platform_load(lib_copy_path_.c_str());
    if (!lib_handle_) return false;

    gameplay_init_     = reinterpret_cast<GwGameplayInitFn>(
        platform_sym(lib_handle_, "gw_gameplay_init"));
    gameplay_update_   = reinterpret_cast<GwGameplayUpdateFn>(
        platform_sym(lib_handle_, "gw_gameplay_update"));
    gameplay_shutdown_ = reinterpret_cast<GwGameplayShutdownFn>(
        platform_sym(lib_handle_, "gw_gameplay_shutdown"));

    return gameplay_init_ && gameplay_update_ && gameplay_shutdown_;
}

void GameplayHost::unload_dll() {
    if (lib_handle_) {
        platform_unload(lib_handle_);
        lib_handle_        = nullptr;
        gameplay_init_     = nullptr;
        gameplay_update_   = nullptr;
        gameplay_shutdown_ = nullptr;
    }
}

bool GameplayHost::enter_play(GameplayContext& ctx) {
    assert(state_ == PIEState::Editor);

    // Snapshot the ECS world (Phase 7 stub — empty snapshot; Phase 8 serializes).
    world_snapshot_.clear();
    // TODO(Phase 8): serialize world into world_snapshot_

    if (!load_dll()) return false;

    gameplay_init_(&ctx);
    state_ = PIEState::Playing;
    return true;
}

void GameplayHost::pause() {
    if (state_ == PIEState::Playing) state_ = PIEState::Paused;
}

void GameplayHost::resume() {
    if (state_ == PIEState::Paused) state_ = PIEState::Playing;
}

void GameplayHost::stop(GameplayContext& ctx) {
    if (state_ == PIEState::Editor) return;

    if (gameplay_shutdown_) gameplay_shutdown_();
    unload_dll();
    state_ = PIEState::Editor;

    // Restore ECS world (Phase 7 stub; Phase 8 deserializes world_snapshot_).
    // TODO(Phase 8): deserialize world_snapshot_ back into ctx.world
    (void)ctx;
}

void GameplayHost::tick(GameplayContext& ctx, float dt) {
    if (state_ == PIEState::Playing && gameplay_update_)
        gameplay_update_(&ctx, dt);
}

void GameplayHost::reload_if_changed() {
    if (state_ != PIEState::Playing) return;
    if (lib_source_path_.empty() || !lib_handle_) return;

    static uint64_t last_mtime = 0;
    uint64_t mtime = platform_mtime(lib_source_path_.c_str());
    if (mtime == 0 || mtime == last_mtime) return;
    last_mtime = mtime;

    // Hot-reload: shutdown, unload, reload, re-init.
    if (gameplay_shutdown_) gameplay_shutdown_();
    unload_dll();
    if (load_dll()) {
        // Reinitialize with the same context (world was not destroyed).
        // This is a stub — Phase 8 will pass the real ctx pointer.
    }
}

}  // namespace gw::editor
