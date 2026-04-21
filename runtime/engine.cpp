// runtime/engine.cpp — ADR-0029 Wave 11F.
//
// Canonical boot order, per ADR-0029 §2:
//   1. CVarRegistry (standard CVars) + event buses
//   2. LocaleBridge + FontLibrary + GlyphAtlas + TextShaper
//   3. AudioService (null backend when deterministic)
//   4. InputService (null backend when deterministic)
//   5. UIService + SettingsBinder
//   6. ConsoleService + built-in commands
// Destructor walks these in reverse.

#include "runtime/engine.hpp"

#include <string>
#include <string_view>

namespace gw::runtime {

namespace {

void seed_default_settings_bindings(ui::SettingsBinder& b) {
    const std::pair<const char*, const char*> kRows[] = {
        {"settings-master-volume", "audio.master.volume"},
        {"settings-music-volume",  "audio.music.volume"},
        {"settings-sfx-volume",    "audio.sfx.volume"},
        {"settings-voice-volume",  "audio.voice.volume"},
        {"settings-text-scale",    "ui.text.scale"},
        {"settings-contrast-boost","ui.contrast.boost"},
        {"settings-reduce-motion", "ui.reduce_motion"},
        {"settings-captions",      "ui.caption.enable"},
        {"settings-caption-scale", "ui.caption.scale"},
        {"settings-invert-y",      "input.look.invert_y"},
        {"settings-mouse-sens",    "input.mouse.sensitivity"},
        {"settings-vsync",         "gfx.swapchain.vsync"},
        {"settings-msaa",          "gfx.render.msaa"},
    };
    for (const auto& [el, cv] : kRows) {
        b.bind({el, cv, 0});
    }
}

void seed_english_strings(ui::LocaleBridge& lb) {
    struct KV { const char* key; const char* val; };
    static const KV kEN[] = {
        {"menu.continue",          "Continue"},
        {"menu.newgame",           "New Game"},
        {"menu.settings",          "Settings"},
        {"menu.credits",           "Credits"},
        {"menu.quit",              "Quit"},
        {"settings.title",         "Settings"},
        {"settings.audio",         "Audio"},
        {"settings.video",         "Video"},
        {"settings.input",         "Input"},
        {"settings.accessibility", "Accessibility"},
        {"settings.apply",         "Apply"},
        {"settings.reset",         "Reset defaults"},
        {"settings.back",          "Back"},
        {"hud.health.label",       "Health"},
        {"hud.shield.label",       "Shield"},
        {"hud.fps.suffix",         "fps"},
        {"console.prompt",         ">"},
        {"console.welcome",        "Greywater dev console ready. Type 'help' for commands."},
        {"console.unknown_command","unknown command"},
        {"console.cheats_banner",  "CHEATS ENABLED"},
        {"a11y.focus_announce",    "focused: {0}"},
        {"a11y.caption.start",     "(captioned)"},
    };
    for (const auto& kv : kEN) {
        lb.register_string("en-US", kv.key, kv.val);
    }
}

} // namespace

Engine::Engine(const EngineConfig& cfg) : cfg_(cfg) {
    // --- Tier-0: standard CVars + bus wiring ---
    std_cvars_ = config::register_standard_cvars(cvars_);
    cvars_.set_bus(&bus_cvars_);

    // --- Tier-1: UI substrate ---
    seed_localization_();
    (void)locale_.set_locale(cfg_.locale.empty() ? std::string_view{"en-US"} : std::string_view{cfg_.locale});

    // --- Tier-2: audio ---
    audio::AudioConfig acfg{};
    acfg.deterministic = cfg_.deterministic || cfg_.headless;
    acfg.enable_hrtf   = cvars_.get_bool_or("audio.spatial.hrtf", true);
    audio_ = std::make_unique<audio::AudioService>(acfg);

    // --- Tier-2: input ---
    input_ = std::make_unique<input::InputService>();
    input::InputConfig icfg{};
    icfg.backend = input::InputBackendKind::Null;  // headless/dev default
    (void)input_->initialize(icfg);

    // --- Tier-2: UI services ---
    ui::UIConfig ucfg{};
    ucfg.width_px  = cfg_.width_px;
    ucfg.height_px = cfg_.height_px;
    ucfg.dpi_scale = cvars_.get_f32_or("ui.dpi.scale", 1.0f);
    ui_ = std::make_unique<ui::UIService>(ucfg, cvars_, fonts_, atlas_, shaper_, locale_,
                                          &bus_layout_, &bus_resize_);

    binder_ = std::make_unique<ui::SettingsBinder>(cvars_);
    seed_default_settings_bindings(*binder_);

    // --- Tier-2: console ---
    console::ConsoleService::Config ccfg{};
#if defined(NDEBUG)
    ccfg.release_build = !cvars_.get_bool_or("dev.console.allow_release", false);
#else
    ccfg.release_build = false;
#endif
    console_ = std::make_unique<console::ConsoleService>(cvars_, ccfg, &bus_console_);
    register_console_commands_();
}

Engine::~Engine() = default;

void Engine::seed_localization_() {
    seed_english_strings(locale_);
}

void Engine::register_console_commands_() {
    // The `quit` command pokes the engine via a trampoline stored in a
    // static lambda capture; since register_builtin_commands takes a
    // plain function pointer, we park the Engine* in a thread-local.
    static Engine* s_self = nullptr;
    s_self = this;
    auto quit_trampoline = []() {
        if (s_self) s_self->request_quit();
    };
    console::register_builtin_commands(*console_, quit_trampoline);
}

void Engine::tick(double dt_seconds) {
    // 1) input — pull events from backend, evaluate actions.
    const float now_ms = static_cast<float>(dt_seconds * 1000.0) * static_cast<float>(frame_index_);
    input_->update(now_ms);

    // 2) UI update — drains keyboard events forwarded by the runtime.
    ui_->update(dt_seconds);

    // 3) Audio update — listener pose is identity in headless mode.
    audio::ListenerState listener{};
    audio_->update(dt_seconds, listener);

    // 4) UI render pass — publishes UILayoutComplete.
    ui_->render();

    ++frame_index_;
}

void Engine::run_frames(std::uint32_t frames, double dt_seconds) {
    for (std::uint32_t i = 0; i < frames && !want_quit_; ++i) {
        tick(dt_seconds);
    }
}

} // namespace gw::runtime
