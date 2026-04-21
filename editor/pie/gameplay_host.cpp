// editor/pie/gameplay_host.cpp
// Play-in-editor lifecycle implementation.
//
// OS-specific dynamic-library loading and filesystem access are delegated to
// `engine/platform/` per CLAUDE.md non-negotiable #11 (OS headers only in
// `engine/platform/`). This translation unit contains no direct `<windows.h>`
// or `<dlfcn.h>` includes.

#include "gameplay_host.hpp"

#include "engine/platform/dll.hpp"
#include "engine/platform/fs.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

namespace gw::editor {

namespace {

// Construct a hot-copy filename beside the source library that includes a
// monotonically incrementing counter, preserving the OS-native shared-library
// extension so the loader accepts it.
std::string make_hot_copy_path(const std::string& source, std::uint32_t counter) {
    std::filesystem::path src(source);
    const std::filesystem::path ext = src.extension();
    char tag[32];
    std::snprintf(tag, sizeof(tag), "_hot_%03u", counter);
    std::filesystem::path dst = src;
    dst.replace_filename(src.stem().string() + tag + ext.string());
    return dst.string();
}

}  // namespace

GameplayHost::~GameplayHost() {
    if (lib_handle_) unload_dll();
}

bool GameplayHost::load_dll() {
    if (lib_source_path_.empty()) return false;

    lib_copy_path_ = make_hot_copy_path(lib_source_path_, hot_copy_counter_++);

    if (!gw::platform::FileSystem::copy_file_overwrite(lib_source_path_, lib_copy_path_)) {
        return false;
    }

    auto lib = std::make_unique<gw::platform::DynamicLibrary>();
    if (!lib->open(lib_copy_path_)) {
        return false;
    }

    gameplay_init_     = reinterpret_cast<GwGameplayInitFn>(
        lib->find_symbol("gw_gameplay_init"));
    gameplay_update_   = reinterpret_cast<GwGameplayUpdateFn>(
        lib->find_symbol("gw_gameplay_update"));
    gameplay_shutdown_ = reinterpret_cast<GwGameplayShutdownFn>(
        lib->find_symbol("gw_gameplay_shutdown"));

    if (!(gameplay_init_ && gameplay_update_ && gameplay_shutdown_)) {
        return false;  // unique_ptr destructor closes the library cleanly
    }

    lib_handle_ = std::move(lib);
    return true;
}

void GameplayHost::unload_dll() {
    if (lib_handle_) {
        lib_handle_.reset();  // DynamicLibrary dtor closes the OS handle
        gameplay_init_     = nullptr;
        gameplay_update_   = nullptr;
        gameplay_shutdown_ = nullptr;
    }
}

bool GameplayHost::enter_play(GameplayContext& ctx) {
    // Guard: transition is only valid from Editor → Playing. Returns false
    // (recoverable) rather than asserting — editor code must not take the
    // sandbox `assert-on-error` path (see docs/12 §B2: assertions are
    // sandbox-only). The UI disables Play while already in PIE, so reaching
    // this branch is a programming error the caller can handle.
    if (state_ != PIEState::Editor) return false;

    // Snapshot the ECS world (Phase 7 stub — empty snapshot; Phase 8 serializes).
    world_snapshot_.clear();
    // TODO(Phase 8): serialize world into world_snapshot_

    if (!load_dll()) return false;

    live_ctx_ = &ctx;
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
    state_     = PIEState::Editor;
    live_ctx_  = nullptr;

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

    static std::uint64_t last_mtime = 0u;
    const std::uint64_t mtime = gw::platform::FileSystem::last_write_stamp(lib_source_path_);
    if (mtime == 0u || mtime == last_mtime) return;
    last_mtime = mtime;

    // Hot-reload: shutdown, unload, reload, re-init with the cached context.
    if (gameplay_shutdown_) gameplay_shutdown_();
    unload_dll();
    if (!load_dll()) {
        // Reload failed — fall back to Editor state rather than leaving
        // ourselves in a "Playing but no DLL" limbo.
        state_    = PIEState::Editor;
        live_ctx_ = nullptr;
        return;
    }
    if (live_ctx_ && gameplay_init_) {
        gameplay_init_(live_ctx_);
    }
}

}  // namespace gw::editor
