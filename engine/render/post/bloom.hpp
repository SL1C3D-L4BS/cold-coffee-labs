#pragma once
// engine/render/post/bloom.hpp — ADR-0079 Wave 17F: dual-Kawase.

#include <cstdint>
#include <vector>

namespace gw::render::post {

struct BloomConfig {
    bool          enabled{true};
    std::uint32_t iterations{5};    // 0..6
    float         threshold{1.0f};
    float         intensity{1.0f};
};

// Dual-Kawase bloom reference implementation on CPU (small image). Used by
// unit tests to verify determinism + intensity round-trip. The GPU path is
// wired behind `engine_render` when a device is present.
class Bloom {
public:
    Bloom() = default;

    void configure(BloomConfig c) noexcept { cfg_ = c; }
    [[nodiscard]] const BloomConfig& config() const noexcept { return cfg_; }

    // Produces a bloom image for the given HDR RGBA32F input. Output
    // has the same size; bloom is added back pre-tonemap. Deterministic.
    void run(const std::vector<float>& hdr,
             std::uint32_t width,
             std::uint32_t height,
             std::vector<float>& out) const;

private:
    BloomConfig cfg_{};
};

} // namespace gw::render::post
