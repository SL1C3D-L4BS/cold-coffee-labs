// editor/pie/gameplay_host.cpp
// Play-in-editor lifecycle implementation.
//
// OS-specific dynamic-library loading and filesystem access are delegated to
// `engine/platform/` per CLAUDE.md non-negotiable #11 (OS headers only in
// `engine/platform/`). This translation unit contains no direct `<windows.h>`
// or `<dlfcn.h>` includes.

#include "gameplay_host.hpp"

#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"
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

bool GameplayHost::take_snapshot(gw::ecs::World& world) {
    world_snapshot_ = gw::ecs::save_world(world, gw::ecs::SnapshotMode::PieSnapshot);
    return !world_snapshot_.empty();
}

bool GameplayHost::restore_snapshot(gw::ecs::World& world) {
    if (world_snapshot_.empty()) return false;
    // load_world clears `world` itself before applying the payload, so we
    // don't need a separate clear step here. CRC verification is on; if the
    // snapshot is corrupt the world ends up empty (via the internal clear)
    // which is safer than a half-populated state.
    auto r = gw::ecs::load_world(world,
                                  std::span<const std::uint8_t>(world_snapshot_),
                                  gw::ecs::LoadOptions{});
    return r.has_value();
}

bool GameplayHost::enter_play(GameplayContext& ctx) {
    // Guard: transition is only valid from Editor → Playing. Returns false
    // (recoverable) rather than asserting — editor code must not take the
    // sandbox `assert-on-error` path (see docs/03_PHILOSOPHY_AND_ENGINEERING.md §B2: assertions are
    // sandbox-only). The UI disables Play while already in PIE, so reaching
    // this branch is a programming error the caller can handle.
    if (state_ != PIEState::Editor) return false;

    // Snapshot first — if we can't round-trip the scene, refuse to enter PIE
    // rather than let the user stranded with no way back. `ctx.world` is the
    // authoring world; it must be non-null for a meaningful PIE session.
    auto* world = static_cast<gw::ecs::World*>(ctx.world);
    if (!world) return false;
    if (!take_snapshot(*world)) return false;

    // Loading the gameplay dll is optional: without it, PIE still runs the
    // simulation loop with whatever systems the editor owns, which is useful
    // for iterating on scene state before a gameplay module exists.
    if (!lib_source_path_.empty()) {
        if (!load_dll()) {
            // DLL is required when a path was configured — we can't silently
            // run without it if the user expected gameplay. Roll back cleanly.
            world_snapshot_.clear();
            return false;
        }
    }

    live_ctx_ = &ctx;
    if (gameplay_init_) gameplay_init_(&ctx);
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

    if (auto* world = static_cast<gw::ecs::World*>(ctx.world)) {
        // Best-effort restore — if the snapshot is corrupt (shouldn't happen
        // unless memory was tampered with), world is left cleared rather than
        // in a partially-overwritten state. The editor surfaces this through
        // the console panel once logging is wired into gameplay_host.
        (void)restore_snapshot(*world);
    }
    world_snapshot_.clear();
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
