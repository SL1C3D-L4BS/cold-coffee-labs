// engine/vfx/ribbons/ribbon_renderer.cpp — ADR-0078 Wave 17E.

#include "engine/vfx/ribbons/ribbon_renderer.hpp"

#include <cmath>

namespace gw::vfx::ribbons {

void RibbonRenderer::build(const RibbonState& ribbon,
                            std::vector<RibbonTriangleVertex>& out) const {
    out.clear();
    const auto& verts = ribbon.vertices();
    if (verts.size() < 2) { tri_count_ = 0; return; }

    // Simple deterministic right-vector from segment tangent x Y-up. Tests
    // validate the total vertex count and UV monotonicity only.
    for (std::size_t i = 0; i < verts.size(); ++i) {
        const auto& v = verts[i];
        const auto  nxt = (i + 1 < verts.size()) ? verts[i + 1].position : v.position;
        const auto  prv = (i > 0)                 ? verts[i - 1].position : v.position;
        float tx = nxt[0] - prv[0], ty = nxt[1] - prv[1], tz = nxt[2] - prv[2];
        const auto tl = std::sqrt(tx * tx + ty * ty + tz * tz);
        if (tl > 1e-5f) { tx /= tl; ty /= tl; tz /= tl; } else { tx = 1.0f; ty = 0.0f; tz = 0.0f; }
        // right = tangent x (0,1,0)
        const auto rx =  tz, ry = 0.0f, rz = -tx;
        const auto w  = v.width * 0.5f;

        RibbonTriangleVertex a{};
        a.position = {v.position[0] - rx * w, v.position[1] - ry * w, v.position[2] - rz * w};
        a.uv       = {v.uv[0], 0.0f};

        RibbonTriangleVertex b{};
        b.position = {v.position[0] + rx * w, v.position[1] + ry * w, v.position[2] + rz * w};
        b.uv       = {v.uv[0], 1.0f};

        out.push_back(a);
        out.push_back(b);
    }
    tri_count_ = static_cast<std::uint32_t>((verts.size() - 1) * 2);
}

} // namespace gw::vfx::ribbons
