#pragma once
// engine/core/config/standard_cvars.hpp — the ~40 CVars that boot with
// every Greywater process. These seed `graphics.toml`, `audio.toml`,
// `input.toml`, `ui.toml`, and `dev.toml`.
//
// The specific names here are the stable surface every other subsystem
// looks up by string. Changing a name is an ADR-worthy change.

#include "engine/core/config/cvar_registry.hpp"

namespace gw::config {

struct StandardCVars {
    // --- Graphics ---
    CVarRef<bool>         gfx_vsync;
    CVarRef<bool>         gfx_debug_wireframe;
    CVarRef<bool>         gfx_debug_overdraw;
    CVarRef<std::int32_t> gfx_msaa;               // 1, 2, 4, 8
    CVarRef<std::int32_t> gfx_resolution_scale;   // percent, 25..200
    // --- Audio ---
    CVarRef<float>        audio_master_volume;
    CVarRef<float>        audio_sfx_volume;
    CVarRef<float>        audio_music_volume;
    CVarRef<float>        audio_voice_volume;
    CVarRef<float>        audio_music_duck_db;
    CVarRef<bool>         audio_hrtf;
    CVarRef<std::int32_t> audio_downmix_mode;     // enum: 0 stereo, 1 mono, 2 bypass
    // --- Input ---
    CVarRef<float>        input_gamepad_deadzone_inner;
    CVarRef<float>        input_gamepad_deadzone_outer;
    CVarRef<float>        input_mouse_sensitivity;
    CVarRef<bool>         input_invert_y;
    CVarRef<std::string>  input_active_profile;
    CVarRef<bool>         input_hotplug_enable;
    // --- UI ---
    CVarRef<float>        ui_text_scale;          // 0.75..2.00
    CVarRef<bool>         ui_contrast_boost;
    CVarRef<float>        ui_dpi_scale;           // 0.5..4.0
    CVarRef<std::string>  ui_locale;              // "en-US", "fr-FR", "ja-JP"
    CVarRef<bool>         ui_caption_enable;
    CVarRef<float>        ui_caption_scale;
    CVarRef<bool>         ui_osk_enabled;         // on-screen keyboard (Phase 16 toggle)
    CVarRef<bool>         ui_reduce_motion;
    CVarRef<std::int32_t> ui_focus_ring_px;
    // --- Dev / console ---
    CVarRef<bool>         dev_console_allow_release;
    CVarRef<bool>         dev_console_allow_cheats;
    CVarRef<bool>         dev_show_fps;
    CVarRef<bool>         dev_show_tracy_zones;
    // --- Net (placeholder for Phase 14, land now to round-trip) ---
    CVarRef<std::int32_t> net_simulate_latency_ms;
    CVarRef<float>        net_simulate_packet_loss;
    // --- Time / RNG ---
    CVarRef<double>       time_scale;
    CVarRef<std::int64_t> rng_seed;
};

// Register every standard CVar into `r`. Idempotent: if a CVar of the
// same name already exists, the existing entry is reused.
[[nodiscard]] StandardCVars register_standard_cvars(CVarRegistry& r);

} // namespace gw::config
