#pragma once
// editor/pie/gameplay_host.hpp
// Play-in-editor lifecycle: load gameplay dll, snapshot/restore ECS world.
// Spec ref: Phase 7 §12.
//
// Platform-specific LoadLibrary/dlopen lives in gameplay_host_platform.cpp.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace gw::editor {

// Minimal context passed to gameplay — a narrow POD struct.
// Every pointer is owned by the engine; gameplay never frees them.
struct GameplayContext {
    uint32_t version       = 1;   // bumped on breaking changes
    void*    world         = nullptr;  // gw::ecs::World* (Phase 8)
    void*    asset_db      = nullptr;  // gw::assets::AssetDatabase*
    void*    input_state   = nullptr;  // gw::input::InputState* (Phase 10 stub)
};

// C-ABI contract — gameplay module must export these three symbols.
using GwGameplayInitFn     = void (*)(GameplayContext*);
using GwGameplayUpdateFn   = void (*)(GameplayContext*, float dt);
using GwGameplayShutdownFn = void (*)();

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
    // Snapshots world, loads dll, calls gw_gameplay_init.
    // Returns false if the dll cannot be found/loaded.
    [[nodiscard]] bool enter_play(GameplayContext& ctx);

    void pause();
    void resume();

    // Transition: Playing/Paused → Editor.
    // Calls gw_gameplay_shutdown, unloads dll, restores world snapshot.
    void stop(GameplayContext& ctx);

    // Advance the gameplay simulation one tick.
    void tick(GameplayContext& ctx, float dt);

    // Check if the gameplay dll was recompiled; hot-reload if so.
    void reload_if_changed();

    [[nodiscard]] PIEState state() const noexcept { return state_; }
    [[nodiscard]] bool in_play()   const noexcept {
        return state_ != PIEState::Editor;
    }

private:
    bool load_dll();
    void unload_dll();

    PIEState    state_             = PIEState::Editor;
    void*       lib_handle_        = nullptr;
    std::string lib_source_path_;
    std::string lib_copy_path_;
    uint32_t    hot_copy_counter_  = 0;

    GwGameplayInitFn     gameplay_init_     = nullptr;
    GwGameplayUpdateFn   gameplay_update_   = nullptr;
    GwGameplayShutdownFn gameplay_shutdown_ = nullptr;

    // ECS world snapshot taken at enter_play — restored at stop.
    // Phase 7 stores a raw byte snapshot; Phase 8 uses the real serializer.
    std::vector<uint8_t> world_snapshot_;
};

}  // namespace gw::editor
