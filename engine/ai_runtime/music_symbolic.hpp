#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gw::ai_runtime {

/// Symbolic adaptive music — replaces the earlier LSTM design.
/// A ≤1M-parameter RoPE transformer selects per-frame stem mix weights on CPU.
/// Presentation-only: excluded from the replay hash (ADR-0098).
/// Budget: ≤ 0.20 ms/frame on RX 580.

inline constexpr std::size_t MUSIC_STEM_COUNT  = 6;
inline constexpr float       MUSIC_BUDGET_MS   = 0.20f;

struct MusicContext {
    float       combat_intensity   = 0.f;   // 0..1
    float       sin_ratio          = 0.f;
    float       rapture_active     = 0.f;   // 1 if in Rapture, else 0
    float       ruin_active        = 0.f;
    float       bpm_beat_phase     = 0.f;   // 0..1 current beat phase
    std::uint32_t circle_index     = 0;
};

struct MusicStemMix {
    std::array<float, MUSIC_STEM_COUNT> weights{};
    float bpm_sync_multiplier = 1.f;
};

[[nodiscard]] MusicStemMix evaluate_music_symbolic(const MusicContext& ctx) noexcept;

} // namespace gw::ai_runtime
