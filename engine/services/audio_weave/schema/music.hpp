#pragma once

#include <array>
#include <cstdint>

namespace gw::services::audio_weave {

inline constexpr std::size_t MAX_STEMS = 8;

struct MusicRequest {
    std::uint64_t seed             = 0;
    std::uint32_t profile_index    = 0;
    float         intensity        = 0.f;
    float         bpm_phase        = 0.f;
    bool          rapture_active   = false;
    bool          ruin_active      = false;
};

struct MusicResult {
    std::array<float, MAX_STEMS> stem_weights{};
    float                        bpm_sync_mul = 1.f;
    std::uint8_t                 active_stem_count = 0;
};

} // namespace gw::services::audio_weave
