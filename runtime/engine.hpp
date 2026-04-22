#pragma once
// runtime/engine.hpp — Phase 11 Wave 11F (ADR-0029).
//
// The `Engine` class is the *single* RAII owner of every runtime subsystem.
// Construction order is the canonical boot sequence defined in ADR-0029 §2.
// Destruction runs it in reverse.
//
// Headers pulled here are all "engine-public" — no OS, no RmlUi, no audio
// backend headers. Implementation lives in runtime/engine.cpp.

#include "engine/a11y/a11y_cvars.hpp"
#include "engine/a11y/a11y_world.hpp"
#include "engine/audio/audio_service.hpp"
#include "engine/audio/audio_types.hpp"
#include "engine/console/console_service.hpp"
#include "engine/core/config/cvar_registry.hpp"
#include "engine/core/config/standard_cvars.hpp"
#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_core.hpp"
#include "engine/i18n/i18n_cvars.hpp"
#include "engine/i18n/i18n_world.hpp"
#include "engine/input/input_service.hpp"
#include "engine/input/input_types.hpp"
#include "engine/persist/persist_cvars.hpp"
#include "engine/persist/persist_world.hpp"
#include "engine/physics/physics_cvars.hpp"
#include "engine/physics/physics_world.hpp"
#include "engine/platform_services/platform_cvars.hpp"
#include "engine/platform_services/platform_services_world.hpp"
#include "engine/render/material/material_cvars.hpp"
#include "engine/render/material/material_world.hpp"
#include "engine/render/post/post_cvars.hpp"
#include "engine/render/post/post_world.hpp"
#include "engine/telemetry/telemetry_world.hpp"
#include "engine/vfx/particles/particle_world.hpp"
#include "engine/vfx/vfx_cvars.hpp"
#include "engine/ui/font_library.hpp"
#include "engine/ui/glyph_atlas.hpp"
#include "engine/ui/locale_bridge.hpp"
#include "engine/ui/settings_binder.hpp"
#include "engine/ui/text_shaper.hpp"
#include "engine/ui/ui_service.hpp"

#include <memory>
#include <string>

namespace gw::runtime {

struct EngineConfig {
    // Headless / self-test knob. In self-test mode the engine:
    //   * uses null audio + null input backends (deterministic),
    //   * skips window creation (UI renders into a virtual surface),
    //   * ticks a fixed number of frames then exits.
    bool        headless{false};
    bool        deterministic{false};   // forces null backends + pinned RNG seed

    // Initial viewport (ignored in headless).
    std::uint32_t width_px{1920};
    std::uint32_t height_px{1080};

    // TOML domain roots. Empty = in-memory registry only.
    std::string  config_dir{};

    // Self-test: how many frames to pump before exit (headless only).
    std::uint32_t self_test_frames{120};

    // Optional BCP-47 tag for UI.  Falls back to "en-US".
    std::string  locale{"en-US"};
};

class Engine {
public:
    explicit Engine(const EngineConfig& cfg);
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Per-frame tick. Ticks in this order (ADR-0029 §3):
    //   1) input.update(now_ms)
    //   2) drain CrossSubsystemBus into engine event buses
    //   3) ui.update(dt)  → dispatches layout
    //   4) audio.update(dt, listener) → renders audio
    //   5) ui.render()                → emits UILayoutComplete
    //   6) bump frame counter
    void tick(double dt_seconds);

    // Run a fixed number of frames and return. Used by the self-test
    // harness and unit tests.
    void run_frames(std::uint32_t frames, double dt_seconds = 1.0 / 60.0);

    // Graceful shutdown request (`quit` console command, OS close, etc.).
    void request_quit() noexcept { want_quit_ = true; }
    [[nodiscard]] bool should_quit() const noexcept { return want_quit_; }

    // -------- Accessors --------
    [[nodiscard]] config::CVarRegistry&  cvars()   noexcept { return cvars_;   }
    [[nodiscard]] ui::UIService&         ui()      noexcept { return *ui_;     }
    [[nodiscard]] console::ConsoleService& console() noexcept { return *console_; }
    [[nodiscard]] audio::AudioService&   audio()   noexcept { return *audio_;  }
    [[nodiscard]] input::InputService&   input()   noexcept { return *input_;  }
    [[nodiscard]] ui::LocaleBridge&      locale()  noexcept { return locale_;  }
    [[nodiscard]] ui::FontLibrary&       fonts()   noexcept { return fonts_;   }
    [[nodiscard]] ui::GlyphAtlas&        atlas()   noexcept { return atlas_;   }
    [[nodiscard]] ui::TextShaper&        shaper()  noexcept { return shaper_;  }
    [[nodiscard]] ui::SettingsBinder&    settings_binder() noexcept { return *binder_; }
    [[nodiscard]] physics::PhysicsWorld& physics() noexcept { return *physics_; }
    [[nodiscard]] const physics::PhysicsWorld& physics() const noexcept { return *physics_; }
    [[nodiscard]] persist::PersistWorld&       persist() noexcept { return *persist_; }
    [[nodiscard]] telemetry::TelemetryWorld&   telemetry() noexcept { return *telemetry_; }
    [[nodiscard]] platform_services::PlatformServicesWorld& platform() noexcept { return *platform_; }
    [[nodiscard]] i18n::I18nWorld&                          i18n() noexcept { return *i18n_; }
    [[nodiscard]] a11y::A11yWorld&                          a11y() noexcept { return *a11y_; }
    [[nodiscard]] render::material::MaterialWorld&          materials() noexcept { return *materials_; }
    [[nodiscard]] vfx::particles::ParticleWorld&            vfx() noexcept { return *vfx_; }
    [[nodiscard]] render::post::PostWorld&                  post() noexcept { return *post_; }

    // Event buses (one per subscribable event type).
    [[nodiscard]] events::EventBus<events::ConfigCVarChanged>&      bus_cvars()   noexcept { return bus_cvars_; }
    [[nodiscard]] events::EventBus<events::ConsoleCommandExecuted>& bus_console() noexcept { return bus_console_; }
    [[nodiscard]] events::EventBus<events::WindowResized>&          bus_resize()  noexcept { return bus_resize_; }
    [[nodiscard]] events::EventBus<events::UILayoutComplete>&       bus_layout()  noexcept { return bus_layout_; }
    [[nodiscard]] events::EventBus<events::DeviceHotplug>&          bus_hotplug() noexcept { return bus_hotplug_; }

    // Stats / self-test accessors.
    [[nodiscard]] std::uint64_t frame_index() const noexcept { return frame_index_; }
    [[nodiscard]] const EngineConfig& config() const noexcept { return cfg_; }

private:
    // Boot helpers — each is a step in the ADR-0029 canonical sequence.
    void seed_localization_();
    void register_console_commands_();

    EngineConfig            cfg_{};
    bool                    want_quit_{false};
    std::uint64_t           frame_index_{0};

    // --- Tier-0: cvars + events (needed by everything below) ---
    config::CVarRegistry                                   cvars_{};
    config::StandardCVars                                  std_cvars_{};
    physics::PhysicsCVars                                  physics_cvars_{};
    persist::PersistCVars                                  persist_cvars_{};
    platform_services::PlatformCVars                       platform_cvars_{};
    i18n::I18nCVars                                        i18n_cvars_{};
    a11y::A11yCVars                                        a11y_cvars_{};
    render::material::MaterialCVars                        material_cvars_{};
    render::material::ShaderCVars                          shader_cvars_{};
    render::post::PostCVars                                post_cvars_{};
    vfx::VfxCVars                                          vfx_cvars_{};
    events::EventBus<events::ConfigCVarChanged>            bus_cvars_{};
    events::EventBus<events::ConsoleCommandExecuted>       bus_console_{};
    events::EventBus<events::WindowResized>                bus_resize_{};
    events::EventBus<events::UILayoutComplete>             bus_layout_{};
    events::EventBus<events::DeviceHotplug>                bus_hotplug_{};

    // --- Tier-1: UI substrate (shaping + atlas + locale) ---
    ui::FontLibrary         fonts_{};
    ui::GlyphAtlas          atlas_{2048};
    ui::TextShaper          shaper_{};
    ui::LocaleBridge        locale_{};

    // --- Tier-2: services (owned by unique_ptr so we control destruction order) ---
    std::unique_ptr<audio::AudioService>       audio_;
    std::unique_ptr<input::InputService>       input_;
    std::unique_ptr<ui::UIService>             ui_;
    std::unique_ptr<ui::SettingsBinder>        binder_;
    std::unique_ptr<console::ConsoleService>   console_;
    std::unique_ptr<physics::PhysicsWorld>     physics_;
    std::unique_ptr<persist::PersistWorld>     persist_;
    std::unique_ptr<telemetry::TelemetryWorld> telemetry_;
    std::unique_ptr<platform_services::PlatformServicesWorld> platform_;
    std::unique_ptr<i18n::I18nWorld>           i18n_;
    std::unique_ptr<a11y::A11yWorld>           a11y_;
    std::unique_ptr<render::material::MaterialWorld>          materials_;
    std::unique_ptr<vfx::particles::ParticleWorld>            vfx_;
    std::unique_ptr<render::post::PostWorld>                  post_;
};

} // namespace gw::runtime
