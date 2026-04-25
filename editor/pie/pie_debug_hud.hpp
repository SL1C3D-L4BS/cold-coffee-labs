#pragma once
// editor/pie/pie_debug_hud.hpp - Part B 12.5 scaffold (ADR-0106).
//
// Toggleable Play-In-Editor overlays. <= 1 ms combined editor-only budget
// on RX 580. Presentation-only; never mutates authoritative state.

namespace gw::editor::pie {

struct PieDebugHudFlags {
    bool martyrdom_meters   = false;  // Sin / Mantra / Rapture / Ruin
    bool god_mode_state     = false;  // Rapture escalation visual
    bool grace_meter        = false;  // Act III only
    bool director_decisions = false;  // state, spawn slot, reward deltas
    bool sin_signature      = false;  // 5-channel fingerprint
    bool frame_timing       = false;  // CPU ms / GPU ms / frame pace
    bool ecs_archetypes     = false;  // live archetype table
};

struct PieDebugHudState {
    PieDebugHudFlags flags{};
    float            budget_ms_measured = 0.f;
    float            budget_ms_cap      = 1.0f;
    // Filled by GameplayHost each PIE tick; presentation only.
    float            dt_last_s          = 0.f;
    unsigned int     entity_count       = 0;
    unsigned char    pie_phase          = 0; // 0 = Playing, 1 = Paused
    bool             gameplay_dll_loaded = false;
};

void draw_pie_debug_hud(PieDebugHudState& state) noexcept;

} // namespace gw::editor::pie
