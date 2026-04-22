#pragma once

#include "engine/world/resources/resource_types.hpp"
#include "engine/world/streaming/chunk_data.hpp"

namespace gw::world::resources {

struct ResourceDistributor {
    /// Minimum edge-to-edge distance enforced between *any* two generated nodes in the same chunk
    /// (independent of `ResourceType`). Exposed for tests / telemetry.
    static constexpr float kPairwiseMinSeparationM = 0.4F;

    [[nodiscard]] static ResourceNodeBatch generate(const gw::world::streaming::HeightmapChunk& chunk,
                                                    const ResourceDensityMap&                 density) noexcept;
};

} // namespace gw::world::resources
