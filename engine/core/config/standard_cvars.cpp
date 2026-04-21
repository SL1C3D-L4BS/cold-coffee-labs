// engine/core/config/standard_cvars.cpp — seeds the registry with the
// ~40 CVars named in ADR-0024 §StandardCVars.

#include "engine/core/config/standard_cvars.hpp"

namespace gw::config {

namespace {

constexpr std::uint32_t kPersistUser =
    kCVarPersist | kCVarUserFacing;
constexpr std::uint32_t kPersistUserRestart =
    kCVarPersist | kCVarUserFacing | kCVarRequiresRestart;

} // namespace

StandardCVars register_standard_cvars(CVarRegistry& r) {
    StandardCVars s;

    // --- Graphics ---
    s.gfx_vsync = r.register_bool({
        "gfx.swapchain.vsync", true, kPersistUser, {}, {}, false,
        "Wait for VBlank before presenting the swapchain.",
    });
    s.gfx_debug_wireframe = r.register_bool({
        "gfx.render.debug_wireframe", false, kCVarDevOnly, {}, {}, false,
        "Force wireframe fill for the main opaque pass (dev only).",
    });
    s.gfx_debug_overdraw = r.register_bool({
        "gfx.render.debug_overdraw", false, kCVarDevOnly, {}, {}, false,
        "Overdraw heatmap in the main opaque pass (dev only).",
    });
    s.gfx_msaa = r.register_i32({
        "gfx.render.msaa", 1, kPersistUserRestart, 1, 8, true,
        "Multi-sample anti-aliasing sample count.",
    });
    s.gfx_resolution_scale = r.register_i32({
        "gfx.render.resolution_scale_pct", 100, kPersistUser, 25, 200, true,
        "Percent of native resolution to render at (dynamic-res upper bound).",
    });

    // --- Audio ---
    s.audio_master_volume = r.register_f32({
        "audio.master.volume", 1.0f, kPersistUser, 0.0f, 1.0f, true,
        "Master bus volume (linear).",
    });
    s.audio_sfx_volume = r.register_f32({
        "audio.sfx.volume", 1.0f, kPersistUser, 0.0f, 1.0f, true,
        "SFX bus volume.",
    });
    s.audio_music_volume = r.register_f32({
        "audio.music.volume", 0.8f, kPersistUser, 0.0f, 1.0f, true,
        "Music bus volume.",
    });
    s.audio_voice_volume = r.register_f32({
        "audio.voice.volume", 1.0f, kPersistUser, 0.0f, 1.0f, true,
        "Voice bus volume.",
    });
    s.audio_music_duck_db = r.register_f32({
        "audio.music.duck_db", 6.0f, kPersistUser, 0.0f, 24.0f, true,
        "Decibels to duck the music bus when a voice line plays.",
    });
    s.audio_hrtf = r.register_bool({
        "audio.spatial.hrtf", true, kPersistUser, {}, {}, false,
        "Enable HRTF-based spatial audio (Steam Audio when available).",
    });
    {
        static const EnumVariant variants[] = {
            {"stereo", 0}, {"mono", 1}, {"bypass", 2},
        };
        s.audio_downmix_mode = r.register_enum(
            "audio.master.downmix", std::span<const EnumVariant>{variants, std::size(variants)},
            0, kPersistUser, "Stereo / mono / bypass final output downmix.");
    }

    // --- Input ---
    s.input_gamepad_deadzone_inner = r.register_f32({
        "input.gamepad.deadzone.inner", 0.10f, kPersistUser, 0.0f, 0.5f, true,
        "Inner radial deadzone (ignored input around the stick center).",
    });
    s.input_gamepad_deadzone_outer = r.register_f32({
        "input.gamepad.deadzone.outer", 0.95f, kPersistUser, 0.5f, 1.0f, true,
        "Outer radial deadzone (saturate to 1.0 past this radius).",
    });
    s.input_mouse_sensitivity = r.register_f32({
        "input.mouse.sensitivity", 1.0f, kPersistUser, 0.1f, 10.0f, true,
        "Global mouse sensitivity multiplier.",
    });
    s.input_invert_y = r.register_bool({
        "input.look.invert_y", false, kPersistUser, {}, {}, false,
        "Invert Y axis on look actions.",
    });
    s.input_active_profile = r.register_string({
        "input.active_profile", std::string{"default"}, kPersistUser, {}, {}, false,
        "Name of the currently active input profile.",
    });
    s.input_hotplug_enable = r.register_bool({
        "input.hotplug.enable", true, kCVarPersist, {}, {}, false,
        "Listen for device-hotplug events.",
    });

    // --- UI ---
    s.ui_text_scale = r.register_f32({
        "ui.text.scale", 1.0f, kPersistUser, 0.75f, 2.0f, true,
        "UI text scale multiplier.",
    });
    s.ui_contrast_boost = r.register_bool({
        "ui.contrast.boost", false, kPersistUser, {}, {}, false,
        "High-contrast theme tokens (WCAG 2.2 AA enforced).",
    });
    s.ui_dpi_scale = r.register_f32({
        "ui.dpi.scale", 1.0f, kPersistUser, 0.5f, 4.0f, true,
        "DPI scale override; 0 = auto-detect (per-monitor on Windows).",
    });
    s.ui_locale = r.register_string({
        "ui.locale", std::string{"en-US"}, kPersistUser, {}, {}, false,
        "Active UI locale; affects LocaleBridge StringId resolution.",
    });
    s.ui_caption_enable = r.register_bool({
        "ui.caption.enable", true, kPersistUser, {}, {}, false,
        "Show captions for audio cues on the caption channel.",
    });
    s.ui_caption_scale = r.register_f32({
        "ui.caption.scale", 1.0f, kPersistUser, 0.5f, 2.0f, true,
        "Caption text scale multiplier.",
    });
    s.ui_osk_enabled = r.register_bool({
        "ui.osk.enabled", false, kPersistUser, {}, {}, false,
        "Show on-screen virtual keyboard when focusing a text input.",
    });
    s.ui_reduce_motion = r.register_bool({
        "ui.reduce_motion", false, kPersistUser, {}, {}, false,
        "Reduce large motion transitions in menus.",
    });
    s.ui_focus_ring_px = r.register_i32({
        "ui.focus_ring_px", 2, kPersistUser, 0, 8, true,
        "Thickness of the keyboard focus ring in CSS pixels.",
    });

    // --- Dev / console ---
    s.dev_console_allow_release = r.register_bool({
        "dev.console.allow_release", false, kCVarDevOnly, {}, {}, false,
        "In release builds, permit the tilde toggle to open the console.",
    });
    s.dev_console_allow_cheats = r.register_bool({
        "dev.console.allow_cheats", false, kCVarCheat, {}, {}, false,
        "Enable cheat CVars (trips 'CHEATS ENABLED' banner when set).",
    });
    s.dev_show_fps = r.register_bool({
        "dev.show_fps", false, kCVarPersist, {}, {}, false,
        "Display a frame-time overlay in the top-right corner.",
    });
    s.dev_show_tracy_zones = r.register_bool({
        "dev.tracy.zones", false, kCVarDevOnly, {}, {}, false,
        "Emit Tracy zones around per-frame scopes (dev only).",
    });

    // --- Net (Phase 14 placeholder) ---
    s.net_simulate_latency_ms = r.register_i32({
        "net.simulate.latency_ms", 0, kCVarDevOnly, 0, 1000, true,
        "Simulated one-way network latency in ms (dev only).",
    });
    s.net_simulate_packet_loss = r.register_f32({
        "net.simulate.packet_loss", 0.0f, kCVarDevOnly, 0.0f, 1.0f, true,
        "Simulated packet-loss probability [0,1] (dev only).",
    });

    // --- Time / RNG ---
    s.time_scale = r.register_f64({
        "time.scale", 1.0, kCVarDevOnly, 0.0, 8.0, true,
        "Global time-scale multiplier (dev only).",
    });
    s.rng_seed = r.register_i64({
        "rng.seed", static_cast<std::int64_t>(0xC01DC0FFEEULL), kCVarPersist,
        0, 0, false,
        "Global RNG seed; deterministic if pinned.",
    });

    return s;
}

} // namespace gw::config
