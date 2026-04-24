#pragma once
// editor/pie/gameplay_host.hpp
// Play-in-editor lifecycle: load gameplay dll, snapshot/restore ECS world.
// Spec ref: Phase 7 §12.
//
// Platform-specific LoadLibrary/dlopen lives in gameplay_host_platform.cpp.

#include "editor/pie/pie_debug_hud.hpp"
#include "editor/pie/pie_perf_guard.hpp"
#include "editor/pie/rollback_inspector.hpp"
#include "engine/play/gameplay_context_abi.hpp"
#include "engine/platform/dll.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gw::ecs { class World; }

namespace gw::editor {

// Shared ABI: `engine/play/gameplay_context_abi.hpp` (append-only; versioned).
using GameplayContext = gw::play::GameplayContext;

// C-ABI contract — gameplay module must export these three symbols.
using GwGameplayInitFn     = void (*)(GameplayContext*);
using GwGameplayUpdateFn   = void (*)(GameplayContext*, float dt);
using GwGameplayShutdownFn = void (*)();
/// Optional — binds `gw::ecs::World*` for PIE (exported as `gameplay_bind_ecs_world`).
using GwGameplayBindWorldFn = void (*)(void* world);

// ---------------------------------------------------------------------------
class GameplayHost {
public:
    enum class PIEState { Editor, Playing, Paused };

    GameplayHost() = default;
    ~GameplayHost();

    GameplayHost(const GameplayHost&)            = delete;
    GameplayHost& operator=(const GameplayHost&) = delete;

    // Path to the gameplay shared library (set once on startup).
    void set_lib_path(std::string path) { lib_source_path_ = std::move(path); }

    // Transition: Editor → Playing.
    // Snapshots world via ecs::save_world, loads dll (optional — PIE may enter
    // without a gameplay module when the editor wants to iterate on scene
    // state in isolation), calls gw_gameplay_init if a dll loaded.
    // Returns false if ctx.world is null or serialization fails — in those
    // cases the editor stays in Editor mode so the user isn't stranded in a
    // PIE state that can't restore.
    [[nodiscard]] bool enter_play(GameplayContext& ctx);

    void pause();
    void resume();

    // Transition: Playing/Paused → Editor.
    // Calls gw_gameplay_shutdown, unloads dll, restores world snapshot via
    // ecs::load_world. Best-effort: if the restore fails the world is cleared
    // and we still return to Editor state rather than deadlocking in PIE.
    void stop(GameplayContext& ctx);

    // Advance the gameplay simulation one tick.
    void tick(GameplayContext& ctx, float dt);

    // Check if the gameplay dll was recompiled; hot-reload if so.
    void reload_if_changed();

    [[nodiscard]] PIEState state() const noexcept { return state_; }
    [[nodiscard]] bool in_play()   const noexcept {
        return state_ != PIEState::Editor;
    }

    // ----- Snapshot/restore (public for tests and scripted automation) -----
    // These run independently of the dll load path so unit tests can exercise
    // the serialization round-trip without a gameplay module on disk.

    // Serialize `world` into the internal snapshot buffer. Returns false if
    // serialization fails (e.g. non-trivially-copyable component present).
    [[nodiscard]] bool take_snapshot(gw::ecs::World& world);

    // Deserialize the internal snapshot back into `world`, clearing whatever
    // was there first. Returns false if the buffer is empty or corrupt; in
    // that case `world` has been cleared but not repopulated.
    [[nodiscard]] bool restore_snapshot(gw::ecs::World& world);

    // Size of the last take_snapshot() payload (0 if none).
    [[nodiscard]] std::size_t snapshot_size() const noexcept {
        return world_snapshot_.size();
    }

private:
    bool load_dll();
    void unload_dll();

    PIEState    state_             = PIEState::Editor;
    std::unique_ptr<gw::platform::DynamicLibrary> lib_handle_;
    std::string lib_source_path_;
    std::string lib_copy_path_;
    uint32_t    hot_copy_counter_  = 0;

    GwGameplayInitFn     gameplay_init_     = nullptr;
    GwGameplayUpdateFn   gameplay_update_   = nullptr;
    GwGameplayShutdownFn gameplay_shutdown_ = nullptr;
    GwGameplayBindWorldFn gameplay_bind_world_ = nullptr;

    // Cached pointer to the last GameplayContext handed to enter_play().
    // Used by reload_if_changed() so hot-reloaded code can be re-initialised
    // against the same context the original load saw. The context is owned
    // by EditorApplication; this is a non-owning reference.
    GameplayContext* live_ctx_ = nullptr;

    // ECS world snapshot taken at enter_play — restored at stop.
    // Phase 7 stores a raw byte snapshot; Phase 8 uses the real serializer.
    std::vector<uint8_t> world_snapshot_;

    // pre-ed-pie-overlays (Part B §12.5 wire-up): editor-only overlays that
    // observe but never mutate the simulation. Scaffold-level today — real
    // render bodies land in Phase 22.
    gw::editor::pie::PieDebugHudState       pie_debug_hud_{};
    gw::editor::pie::PiePerfGuardState      pie_perf_guard_{};
    gw::editor::pie::RollbackInspectorState rollback_inspector_{};
};

}  // namespace gw::editor
