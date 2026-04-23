#pragma once
// gameplay/a11y/gameplay_a11y.hpp — Part C §24 scaffold (ADR-0116, WCAG 2.2 AA).
//
// Sacrilege-specific accessibility layer that sits on top of engine/a11y
// (ADR-0071). Exposes gameplay-only toggles that do not fit the generic
// engine contract: gore slider, photosensitivity warning, input remap,
// per-ability audio sliders. Non-deterministic presentation, not simulation —
// gameplay/martyrdom and engine/ecs remain the source of truth.

#include <cstddef>
#include <cstdint>

namespace gw::gameplay::a11y {

// ---------------------------------------------------------------------------
// Colour-blind modes (gameplay-facing). Engine already provides a colour
// matrix pipeline (engine/a11y/color_matrices); the gameplay layer picks a
// named preset per encounter and can override it for the Sin Signature HUD.
// ---------------------------------------------------------------------------
enum class ColorBlindMode : std::uint8_t {
    None       = 0,
    Protanopia = 1,
    Deuteranopia = 2,
    Tritanopia = 3,
    Achromatopsia = 4,
};

// Gore slider — graduated violence presentation. Simulation damage is not
// affected; only decals, particle counts, and dismemberment visuals.
enum class GoreLevel : std::uint8_t {
    Off       = 0,
    Reduced   = 1,
    Standard  = 2,
    Unleashed = 3, // default for Sacrilege.
};

// Photosensitivity warning + clamp. Caps strobe frequency (≤ 3 Hz), disables
// full-screen red flashes, softens God Mode glitch FX. WCAG 2.3.1/2.3.2.
struct PhotosensitivityProfile {
    bool  warning_shown        = false;
    bool  strobe_clamp_enabled = true;  // on by default.
    float max_strobe_hz        = 3.0F;
    bool  disable_red_flash    = true;
    bool  soften_god_mode      = false;
};

// Subtitles beyond the engine floor: speaker colour for Malakor/Niccolò,
// on-screen position preference (top/bottom/follow-camera), force-directional
// arrow for off-screen speakers.
enum class SubtitlePosition : std::uint8_t {
    Bottom       = 0,
    Top          = 1,
    FollowCamera = 2,
};

struct GameplaySubtitles {
    bool             speaker_color_enabled   = true;
    bool             directional_arrow       = true;
    SubtitlePosition position                = SubtitlePosition::Bottom;
    float            character_reveal_wpm    = 0.0F; // 0 = instant.
};

// Per-bus audio sliders. Independent of master mixer so the player can mute
// voice-of-Logos shouts without losing music. Values in [0,1].
struct AudioSliders {
    float master       = 1.0F;
    float music        = 1.0F;
    float sfx          = 1.0F;
    float ambience     = 1.0F;
    float voice        = 1.0F;
    float ui           = 1.0F;
    float logos_shouts = 1.0F; // Sacrilege-specific; see docs/11 §3.
};

// Input remap — full-action rebind including movement, abilities, circle
// switch, and God Mode trigger. Hooked into engine/input, but gameplay owns
// the mapping table because ability bindings are game-logic.
struct InputBinding {
    std::uint32_t action_id     = 0;
    std::uint32_t primary_key   = 0;
    std::uint32_t secondary_key = 0;
    std::uint32_t gamepad_btn   = 0;
};

constexpr std::size_t MAX_BINDINGS = 64;

struct InputRemapState {
    InputBinding  bindings[MAX_BINDINGS]{};
    std::size_t   binding_count      = 0;
    bool          allow_duplicate    = false;
    bool          rebind_in_progress = false;
};

// Top-level gameplay a11y config (read-only after level load; mutations
// reroute through the settings panel and a deterministic reload token).
struct GameplayA11yConfig {
    ColorBlindMode            color_mode    = ColorBlindMode::None;
    GoreLevel                 gore          = GoreLevel::Unleashed;
    PhotosensitivityProfile   photo;
    GameplaySubtitles         subtitles;
    AudioSliders              audio;
    InputRemapState           remap;
    bool                      high_contrast_hud = false;
    bool                      reduce_camera_shake = false; // motion-sickness.
    bool                      one_handed_controls = false;
};

// Populate defaults at startup; wires into engine/a11y CVars where overlap
// exists (see ADR-0116).
void initialize_defaults(GameplayA11yConfig& cfg) noexcept;

// Show the first-run photosensitivity warning. Returns true once the player
// has acknowledged. Stored in the user settings blob.
bool maybe_show_photosensitivity_warning(GameplayA11yConfig& cfg) noexcept;

// Apply config → runtime: pushes colour matrix, updates gore LOD table,
// audio bus gains, input-map snapshot. Idempotent.
void apply(const GameplayA11yConfig& cfg) noexcept;

} // namespace gw::gameplay::a11y
