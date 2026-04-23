// engine/input/steam_input_glue.cpp — Part C §22 scaffold.
//
// Scaffold returns safe defaults. Real Steamworks integration lands in
// Phase 22 with `GW_ENABLE_STEAM_INPUT=ON`.

#include "engine/input/steam_input_glue.hpp"

namespace gw::input::steam {

namespace {
ActiveScheme g_active = ActiveScheme::KeyboardMouse;
}

bool startup() noexcept {
    // Phase 22: load steam_api64.dll/.so, SteamInput()->Init(...).
    return false;
}

void shutdown() noexcept {}

ActiveScheme tick_active_scheme() noexcept { return g_active; }

GlyphResult resolve_glyph(const GlyphRequest& /*req*/) noexcept {
    GlyphResult r{};
    r.label = "—";
    return r;
}

} // namespace gw::input::steam
