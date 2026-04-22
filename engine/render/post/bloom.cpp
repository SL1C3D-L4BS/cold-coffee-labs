// engine/render/post/bloom.cpp — ADR-0079 Wave 17F dual-Kawase reference.

#include "engine/render/post/bloom.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace gw::render::post {

namespace {

inline std::size_t idx(std::uint32_t x, std::uint32_t y, std::uint32_t w) noexcept {
    return (static_cast<std::size_t>(y) * w + x) * 4u;
}

inline float luma(float r, float g, float b) noexcept {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// 4-tap Kawase blur (separable-ish): sample the four (x±1, y±1) neighbors.
void kawase_pass(const std::vector<float>& src,
                 std::uint32_t sw, std::uint32_t sh,
                 std::vector<float>& dst,
                 std::uint32_t dw, std::uint32_t dh,
                 float offset) noexcept {
    dst.assign(static_cast<std::size_t>(dw) * dh * 4u, 0.0f);
    if (sw == 0 || sh == 0) return;
    const float scale_x = static_cast<float>(sw) / static_cast<float>(dw);
    const float scale_y = static_cast<float>(sh) / static_cast<float>(dh);
    for (std::uint32_t y = 0; y < dh; ++y) {
        for (std::uint32_t x = 0; x < dw; ++x) {
            const float cx = (static_cast<float>(x) + 0.5f) * scale_x - 0.5f;
            const float cy = (static_cast<float>(y) + 0.5f) * scale_y - 0.5f;
            float acc[4] = {0, 0, 0, 0};
            const float ox[4] = {-offset, +offset, -offset, +offset};
            const float oy[4] = {-offset, -offset, +offset, +offset};
            for (int i = 0; i < 4; ++i) {
                const std::int32_t xi = std::clamp(static_cast<std::int32_t>(std::lround(cx + ox[i])),
                                                    0, static_cast<std::int32_t>(sw) - 1);
                const std::int32_t yi = std::clamp(static_cast<std::int32_t>(std::lround(cy + oy[i])),
                                                    0, static_cast<std::int32_t>(sh) - 1);
                const std::size_t si = idx(static_cast<std::uint32_t>(xi),
                                             static_cast<std::uint32_t>(yi), sw);
                acc[0] += src[si + 0];
                acc[1] += src[si + 1];
                acc[2] += src[si + 2];
                acc[3] += src[si + 3];
            }
            const std::size_t di = idx(x, y, dw);
            dst[di + 0] = acc[0] * 0.25f;
            dst[di + 1] = acc[1] * 0.25f;
            dst[di + 2] = acc[2] * 0.25f;
            dst[di + 3] = acc[3] * 0.25f;
        }
    }
}

} // namespace

void Bloom::run(const std::vector<float>& hdr,
                 std::uint32_t width, std::uint32_t height,
                 std::vector<float>& out) const {
    out.assign(hdr.size(), 0.0f);
    if (!cfg_.enabled || cfg_.iterations == 0 || width == 0 || height == 0) {
        out = hdr;
        return;
    }

    // 1) Threshold → bright pass.
    std::vector<float> bright(hdr.size(), 0.0f);
    for (std::size_t p = 0; p < hdr.size(); p += 4) {
        const float l = luma(hdr[p + 0], hdr[p + 1], hdr[p + 2]);
        const float k = std::max(0.0f, l - cfg_.threshold);
        const float w = l > 0.0f ? k / l : 0.0f;
        bright[p + 0] = hdr[p + 0] * w;
        bright[p + 1] = hdr[p + 1] * w;
        bright[p + 2] = hdr[p + 2] * w;
        bright[p + 3] = 1.0f;
    }

    // 2) Downsample ladder (dual-Kawase down pass).
    std::vector<std::vector<float>> chain;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> sizes;
    chain.push_back(bright);
    sizes.emplace_back(width, height);
    std::uint32_t cw = width, ch = height;
    for (std::uint32_t i = 0; i < cfg_.iterations; ++i) {
        const auto nw = std::max<std::uint32_t>(1u, cw / 2);
        const auto nh = std::max<std::uint32_t>(1u, ch / 2);
        std::vector<float> nx;
        kawase_pass(chain.back(), cw, ch, nx, nw, nh, 1.0f);
        chain.push_back(std::move(nx));
        sizes.emplace_back(nw, nh);
        cw = nw; ch = nh;
    }

    // 3) Upsample ladder (dual-Kawase up pass, additive).
    for (std::uint32_t i = cfg_.iterations; i > 0; --i) {
        const auto [dw, dh] = sizes[i - 1];
        std::vector<float> up;
        kawase_pass(chain[i], sizes[i].first, sizes[i].second, up, dw, dh, 0.5f);
        auto& target = chain[i - 1];
        for (std::size_t p = 0; p < target.size(); ++p) target[p] += up[p];
    }

    // 4) Composite back onto HDR with intensity.
    out = hdr;
    for (std::size_t p = 0; p < out.size(); p += 4) {
        out[p + 0] += chain[0][p + 0] * cfg_.intensity;
        out[p + 1] += chain[0][p + 1] * cfg_.intensity;
        out[p + 2] += chain[0][p + 2] * cfg_.intensity;
    }
}

} // namespace gw::render::post
