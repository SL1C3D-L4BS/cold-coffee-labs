#pragma once
// engine/vfx/ribbons/ribbon_renderer.hpp — ADR-0078 Wave 17E.

#include "engine/vfx/ribbons/ribbon_state.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace gw::vfx::ribbons {

// Turns a RibbonState into a triangle-strip vertex buffer using a fixed
// view-direction stand-in (Y-up billboarding) — deterministic for tests.
struct RibbonTriangleVertex {
    std::array<float, 3> position;
    std::array<float, 2> uv;
};

class RibbonRenderer {
public:
    // Expands N nodes → 2N billboard verts + (2N - 2) triangle-strip.
    void build(const RibbonState& ribbon,
                std::vector<RibbonTriangleVertex>& out) const;

    [[nodiscard]] std::uint32_t last_triangle_count() const noexcept { return tri_count_; }

private:
    mutable std::uint32_t tri_count_{0};
};

} // namespace gw::vfx::ribbons
