#pragma once
// engine/world/planet/gptm_lod_selector.hpp — screen-space + distance GPTM LOD selection.

#include "engine/world/planet/gptm_types.hpp"
#include "engine/world/streaming/chunk_coord.hpp"

#include <cstdint>

namespace gw::config {
class CVarRegistry;
}

namespace gw::world::planet {

class GptmLodSelector {
public:
    /// Baseline screen-space error (pixels). Override with CVar `world.gptm.lod_error_px` when `cvars` is non-null.
    static constexpr float kDefaultErrorThresholdPx = 4.0f;

    /// `cvars` optional — when null, `kDefaultErrorThresholdPx` is used.
    [[nodiscard]] static GptmLod select(gw::world::streaming::ChunkCoord coord,
                                        gw::world::streaming::Vec3_f64 camera_world_pos,
                                        float                                 fov_y_rad,
                                        std::uint32_t                        screen_height_px,
                                        const gw::config::CVarRegistry*     cvars = nullptr) noexcept;

    [[nodiscard]] static float error_threshold_px(const gw::config::CVarRegistry* cvars) noexcept;
};

} // namespace gw::world::planet
