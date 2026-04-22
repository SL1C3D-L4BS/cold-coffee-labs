#pragma once
// engine/render/post/taa.hpp — ADR-0079 Wave 17F: TAA + k-DOP clipping.

#include "engine/render/post/post_types.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace gw::render::post {

struct TaaConfig {
    bool          enabled{true};
    TaaClipMode   clip_mode{TaaClipMode::KDop};
    float         jitter_scale{1.0f};
    float         blend_factor{0.90f};     // history weight
};

struct HaltonJitter {
    float x{0.0f};
    float y{0.0f};
};

// Halton low-discrepancy sequence (used as TAA sub-pixel jitter).
[[nodiscard]] HaltonJitter halton23_jitter(std::uint32_t frame_index,
                                             float jitter_scale = 1.0f) noexcept;

// Deterministic color clip for TAA neighborhood clamp.
[[nodiscard]] std::array<float, 3>
clip_color(TaaClipMode mode,
            const std::array<float, 3>& history,
            const std::array<std::array<float, 3>, 9>& neighborhood) noexcept;

class Taa {
public:
    void configure(TaaConfig c) noexcept { cfg_ = c; }
    [[nodiscard]] const TaaConfig& config() const noexcept { return cfg_; }

    // Run TAA against a 9-sample neighborhood from the current frame, blending
    // with the history buffer in place. CPU reference path; determinism gated
    // on HaltonJitter and clip_color.
    [[nodiscard]] std::array<float, 3>
    resolve(const std::array<float, 3>& current,
             const std::array<float, 3>& history,
             const std::array<std::array<float, 3>, 9>& neighborhood) const noexcept;

    // Invalidate history on respawn / camera teleport (unit test hook).
    void invalidate() noexcept { invalidated_ = true; }
    [[nodiscard]] bool invalidated() const noexcept { return invalidated_; }
    void clear_invalidated() noexcept { invalidated_ = false; }

private:
    TaaConfig cfg_{};
    bool      invalidated_{false};
};

} // namespace gw::render::post
