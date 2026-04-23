// engine/world/gptm/gptm_lod_selector.cpp

#include "engine/world/gptm/gptm_lod_selector.hpp"

#include "engine/core/config/cvar_registry.hpp"
#include "engine/world/streaming/chunk_coord.hpp"

#include <algorithm>
#include <cmath>

namespace gw::world::gptm {
namespace {

using gw::world::streaming::CHUNK_SIZE_METERS;
using gw::world::streaming::ChunkCoord;
using gw::world::streaming::Vec3_f64;
using gw::world::streaming::chunk_origin;

[[nodiscard]] GptmLod coarser(GptmLod l) noexcept {
    const auto u = static_cast<std::uint8_t>(l);
    return static_cast<GptmLod>(static_cast<std::uint8_t>(std::min<std::uint8_t>(u + 1, 4)));
}

[[nodiscard]] GptmLod coarser_of(const GptmLod a, const GptmLod b) noexcept {
    return static_cast<GptmLod>(static_cast<std::uint8_t>(std::max(static_cast<std::uint8_t>(a),
                                                                    static_cast<std::uint8_t>(b))));
}

[[nodiscard]] double dist_camera_to_chunk_aabb(const ChunkCoord& c, const Vec3_f64& cam) noexcept {
    const Vec3_f64 mn = chunk_origin(c);
    const Vec3_f64 mx(mn.x() + CHUNK_SIZE_METERS, mn.y() + CHUNK_SIZE_METERS,
                      mn.z() + CHUNK_SIZE_METERS);

    const double cx = std::clamp(cam.x(), mn.x(), mx.x());
    const double cy = std::clamp(cam.y(), mn.y(), mx.y());
    const double cz = std::clamp(cam.z(), mn.z(), mx.z());

    const double dx = cam.x() - cx;
    const double dy = cam.y() - cy;
    const double dz = cam.z() - cz;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

[[nodiscard]] GptmLod lod_from_distance_m(const double d) noexcept {
    if (d >= 2000.0) {
        return GptmLod::Lod4;
    }
    if (d >= 1024.0) {
        return GptmLod::Lod4;
    }
    if (d >= 512.0) {
        return GptmLod::Lod3;
    }
    if (d >= 256.0) {
        return GptmLod::Lod2;
    }
    if (d >= 128.0) {
        return GptmLod::Lod1;
    }
    return GptmLod::Lod0;
}

} // namespace

float GptmLodSelector::error_threshold_px(const gw::config::CVarRegistry* cvars) noexcept {
    if (cvars == nullptr) {
        return kDefaultErrorThresholdPx;
    }
    return cvars->get_f32_or("world.gptm.lod_error_px", kDefaultErrorThresholdPx);
}

GptmLod GptmLodSelector::select(const ChunkCoord coord, const Vec3_f64 camera_world_pos,
                                 const float fov_y_rad, const std::uint32_t screen_height_px,
                                 const gw::config::CVarRegistry* cvars) noexcept {
    const double dist = dist_camera_to_chunk_aabb(coord, camera_world_pos);
    const GptmLod  dist_lod = lod_from_distance_m(dist);

    if (dist > 2000.0) {
        return GptmLod::Lod4;
    }

    const float thresh = error_threshold_px(cvars);
    const float tan_half =
        std::max(1e-4f, std::tan(std::max(1e-4f, fov_y_rad) * 0.5f));
    const float extent_m = static_cast<float>(CHUNK_SIZE_METERS * 1.73205080757);
    const float approx_diag_px =
        (extent_m / static_cast<float>(std::max(dist, 1.0)))
        * (static_cast<float>(std::max(1u, screen_height_px)) / (2.f * tan_half));

    GptmLod lod = GptmLod::Lod0;
    while (lod < GptmLod::Lod4) {
        const std::uint32_t cells = 32u >> static_cast<std::uint32_t>(lod);
        const float         subpx = approx_diag_px / static_cast<float>(std::max(1u, cells));
        if (subpx >= thresh) {
            break;
        }
        lod = coarser(lod);
    }

    return coarser_of(lod, dist_lod);
}

} // namespace gw::world::gptm
