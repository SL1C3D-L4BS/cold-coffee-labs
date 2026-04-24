#pragma once
// engine/render/visibility/render_visibility_policy.hpp — GPU / mesh visibility (plan Track B1).
//
// Distinct from `gw::audio::IOcclusionProvider`: this policy governs what the
// renderer submits, not audio propagation occlusion.

#include <cstdint>

namespace gw::render::visibility {

struct VisibilityQuery {
    std::uint64_t chunk_key = 0;
    float         budget_ms = 4.f;
};

struct VisibilityDecision {
    bool draw   = true;
    int  lod_level = 0;
};

class IRenderVisibilityPolicy {
public:
    virtual ~IRenderVisibilityPolicy() = default;
    virtual VisibilityDecision evaluate(const VisibilityQuery&) const = 0;
};

/// Default: draw everything at LOD0 (until streaming + cluster culling land).
class PassthroughVisibilityPolicy final : public IRenderVisibilityPolicy {
public:
    VisibilityDecision evaluate(const VisibilityQuery&) const override {
        return VisibilityDecision{.draw = true, .lod_level = 0};
    }
};

} // namespace gw::render::visibility
