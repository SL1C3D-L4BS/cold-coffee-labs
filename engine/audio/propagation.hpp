#pragma once
// engine/audio/propagation.hpp — occlusion/obstruction provider (ADR-0018 §2.4).
//
// Phase 10 ships the interface + a null provider + a stub voxel-grid
// provider. Phase 12 replaces the grid provider with a Jolt raycast-based
// implementation without touching this file.

#include "engine/audio/audio_types.hpp"

#include <functional>
#include <memory>

namespace gw::audio {

struct OcclusionQuery {
    math::Vec3d listener_pos;
    math::Vec3d emitter_pos;
};

struct OcclusionResult {
    float occlusion{0.0f};   // 0 = clear, 1 = solid
    float obstruction{0.0f}; // 0 = clear, 1 = partial block
};

class IOcclusionProvider {
public:
    virtual ~IOcclusionProvider() = default;
    virtual OcclusionResult query(const OcclusionQuery&) const = 0;
};

// Always-clear provider — used by default when physics is not up.
[[nodiscard]] std::unique_ptr<IOcclusionProvider> make_null_occlusion();

// Grid/voxel sampler — stub implementation of the Phase-12 raycast provider.
// Callers supply a lambda that, for a given world-space point, returns an
// occlusion density (0..1). The provider marches the listener-to-emitter
// segment at `step_m` and accumulates coverage.
using VoxelSampler = std::function<float(const math::Vec3d&)>;

struct GridVoxelConfig {
    float    step_m{1.0f};
    uint32_t max_steps{128};
    float    obstruction_fraction{0.5f};  // of sampled total becomes obstruction
};

[[nodiscard]] std::unique_ptr<IOcclusionProvider>
make_grid_voxel_occlusion(VoxelSampler sampler, GridVoxelConfig cfg = {});

} // namespace gw::audio
