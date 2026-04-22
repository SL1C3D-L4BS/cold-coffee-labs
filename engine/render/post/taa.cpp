// engine/render/post/taa.cpp — ADR-0079 Wave 17F: TAA + k-DOP clipping.

#include "engine/render/post/taa.hpp"

#include <algorithm>
#include <cmath>

namespace gw::render::post {

namespace {

constexpr float halton(std::uint32_t i, std::uint32_t b) noexcept {
    float f = 1.0f;
    float r = 0.0f;
    while (i > 0) {
        f /= static_cast<float>(b);
        r += f * static_cast<float>(i % b);
        i /= b;
    }
    return r;
}

} // namespace

HaltonJitter halton23_jitter(std::uint32_t frame_index, float jitter_scale) noexcept {
    const float x = halton(frame_index + 1, 2) - 0.5f;
    const float y = halton(frame_index + 1, 3) - 0.5f;
    return {x * jitter_scale, y * jitter_scale};
}

namespace {

std::array<float, 3> clip_aabb(const std::array<float, 3>& history,
                                 const std::array<std::array<float, 3>, 9>& nb) noexcept {
    std::array<float, 3> lo = nb[0], hi = nb[0];
    for (std::size_t i = 1; i < nb.size(); ++i) {
        for (std::size_t c = 0; c < 3; ++c) {
            lo[c] = std::min(lo[c], nb[i][c]);
            hi[c] = std::max(hi[c], nb[i][c]);
        }
    }
    return {
        std::clamp(history[0], lo[0], hi[0]),
        std::clamp(history[1], lo[1], hi[1]),
        std::clamp(history[2], lo[2], hi[2]),
    };
}

std::array<float, 3> clip_variance(const std::array<float, 3>& history,
                                     const std::array<std::array<float, 3>, 9>& nb) noexcept {
    std::array<float, 3> mean{0, 0, 0};
    std::array<float, 3> m2{0, 0, 0};
    for (const auto& s : nb) for (int c = 0; c < 3; ++c) mean[c] += s[c];
    for (int c = 0; c < 3; ++c) mean[c] /= 9.0f;
    for (const auto& s : nb) for (int c = 0; c < 3; ++c) m2[c] += s[c] * s[c];
    std::array<float, 3> sigma{};
    for (int c = 0; c < 3; ++c) {
        const float var = std::max(0.0f, m2[c] / 9.0f - mean[c] * mean[c]);
        sigma[c] = std::sqrt(var);
    }
    const float gamma = 1.5f;
    return {
        std::clamp(history[0], mean[0] - gamma * sigma[0], mean[0] + gamma * sigma[0]),
        std::clamp(history[1], mean[1] - gamma * sigma[1], mean[1] + gamma * sigma[1]),
        std::clamp(history[2], mean[2] - gamma * sigma[2], mean[2] + gamma * sigma[2]),
    };
}

// k-DOP clip: 7 cardinal axes (3 RGB + 4 luminance/chroma diagonals)
// projected minima/maxima form a tighter convex hull than AABB.
std::array<float, 3> clip_kdop(const std::array<float, 3>& history,
                                 const std::array<std::array<float, 3>, 9>& nb) noexcept {
    struct Axis { std::array<float, 3> dir; };
    static const std::array<Axis, 7> axes{{
        {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f}},
        {{0.57735f,  0.57735f,  0.57735f}},
        {{0.57735f,  0.57735f, -0.57735f}},
        {{0.57735f, -0.57735f,  0.57735f}},
        {{-0.57735f, 0.57735f,  0.57735f}},
    }};

    auto center = nb[4]; // take center tap as bias
    auto dir = std::array<float, 3>{
        history[0] - center[0],
        history[1] - center[1],
        history[2] - center[2],
    };

    float t_max = 1.0f;
    for (const auto& a : axes) {
        const float dd = dir[0] * a.dir[0] + dir[1] * a.dir[1] + dir[2] * a.dir[2];
        if (std::abs(dd) < 1e-6f) continue;
        float lo = 1e30f, hi = -1e30f;
        for (const auto& s : nb) {
            const auto p = (s[0] - center[0]) * a.dir[0]
                          + (s[1] - center[1]) * a.dir[1]
                          + (s[2] - center[2]) * a.dir[2];
            lo = std::min(lo, p);
            hi = std::max(hi, p);
        }
        const float t_lo = dd > 0.0f ? (lo / dd) : (hi / dd);
        const float t_hi = dd > 0.0f ? (hi / dd) : (lo / dd);
        if (t_lo < 0.0f && t_hi > 0.0f) {
            t_max = std::min(t_max, t_hi);
        }
    }
    t_max = std::clamp(t_max, 0.0f, 1.0f);
    return {
        center[0] + dir[0] * t_max,
        center[1] + dir[1] * t_max,
        center[2] + dir[2] * t_max,
    };
}

} // namespace

std::array<float, 3>
clip_color(TaaClipMode mode,
            const std::array<float, 3>& history,
            const std::array<std::array<float, 3>, 9>& nb) noexcept {
    switch (mode) {
        case TaaClipMode::Aabb:     return clip_aabb(history, nb);
        case TaaClipMode::Variance: return clip_variance(history, nb);
        case TaaClipMode::KDop:
        default:                     return clip_kdop(history, nb);
    }
}

std::array<float, 3>
Taa::resolve(const std::array<float, 3>& current,
              const std::array<float, 3>& history,
              const std::array<std::array<float, 3>, 9>& nb) const noexcept {
    if (!cfg_.enabled || invalidated_) return current;
    const auto clipped = clip_color(cfg_.clip_mode, history, nb);
    const float b = cfg_.blend_factor;
    return {
        current[0] * (1.0f - b) + clipped[0] * b,
        current[1] * (1.0f - b) + clipped[1] * b,
        current[2] * (1.0f - b) + clipped[2] * b,
    };
}

} // namespace gw::render::post
