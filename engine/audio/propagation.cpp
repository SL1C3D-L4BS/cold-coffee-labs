// engine/audio/propagation.cpp
#include "engine/audio/propagation.hpp"

#include <algorithm>
#include <cmath>

namespace gw::audio {

namespace {

class NullOcclusion final : public IOcclusionProvider {
public:
    OcclusionResult query(const OcclusionQuery&) const override { return {}; }
};

class GridVoxelOcclusion final : public IOcclusionProvider {
public:
    GridVoxelOcclusion(VoxelSampler s, GridVoxelConfig c)
        : sampler_(std::move(s)), cfg_(c) {}

    OcclusionResult query(const OcclusionQuery& q) const override {
        const math::Vec3d d{q.emitter_pos.x() - q.listener_pos.x(),
                            q.emitter_pos.y() - q.listener_pos.y(),
                            q.emitter_pos.z() - q.listener_pos.z()};
        const double dist = std::sqrt(d.x() * d.x() + d.y() * d.y() + d.z() * d.z());
        if (dist < 1e-6) return {};
        const uint32_t steps = std::min(
            cfg_.max_steps,
            static_cast<uint32_t>(dist / std::max<float>(cfg_.step_m, 1e-3f)));
        if (steps == 0) return {};

        float accum = 0.0f;
        uint32_t hits = 0;
        for (uint32_t i = 1; i <= steps; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(steps + 1);
            math::Vec3d p{q.listener_pos.x() + d.x() * t,
                          q.listener_pos.y() + d.y() * t,
                          q.listener_pos.z() + d.z() * t};
            const float s = sampler_ ? sampler_(p) : 0.0f;
            if (s > 0.0f) {
                accum += s;
                ++hits;
            }
        }
        const float mean = steps > 0 ? accum / static_cast<float>(steps) : 0.0f;
        OcclusionResult r;
        r.occlusion   = std::clamp(mean, 0.0f, 1.0f);
        r.obstruction = std::clamp(mean * cfg_.obstruction_fraction, 0.0f, 1.0f);
        (void) hits;
        return r;
    }

private:
    VoxelSampler     sampler_;
    GridVoxelConfig  cfg_;
};

} // namespace

std::unique_ptr<IOcclusionProvider> make_null_occlusion() {
    return std::make_unique<NullOcclusion>();
}

std::unique_ptr<IOcclusionProvider> make_grid_voxel_occlusion(VoxelSampler sampler, GridVoxelConfig cfg) {
    return std::make_unique<GridVoxelOcclusion>(std::move(sampler), cfg);
}

} // namespace gw::audio
