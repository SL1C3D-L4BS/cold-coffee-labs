#pragma once
// engine/input/steam_input_glue.hpp — Part C §22 scaffold (ADR-0113).
//
// Steam Input API glue. Gated behind `GW_ENABLE_STEAM_INPUT`. Used to
// surface correct glyphs (never M+KB when a Deck / controller is active)
// and to wire default controller bindings that hit every game function
// without menu hunting.

#include <cstdint>

namespace gw::input::steam {

enum class ActiveScheme : std::uint8_t {
    KeyboardMouse = 0,
    XboxPad       = 1,
    PlaystationPad= 2,
    SteamDeck     = 3,
    SwitchPro     = 4,
    Generic       = 5,
};

struct GlyphRequest {
    const char*  action_id   = nullptr;
    ActiveScheme scheme      = ActiveScheme::KeyboardMouse;
};

struct GlyphResult {
    const char*  texture_path = nullptr;   // cooked `.gwtex` path
    const char*  label        = nullptr;   // fallback text
};

/// Initialise Steam Input. Returns false if Steamworks is unavailable.
bool startup() noexcept;
void shutdown() noexcept;

/// Called per-frame to update scheme detection based on the last input
/// origin Steam reported.
ActiveScheme tick_active_scheme() noexcept;

/// Resolve a glyph for the active scheme. Never returns null.
GlyphResult resolve_glyph(const GlyphRequest& req) noexcept;

} // namespace gw::input::steam
