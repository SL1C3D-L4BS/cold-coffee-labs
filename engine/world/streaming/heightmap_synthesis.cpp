#include "engine/world/streaming/heightmap_synthesis.hpp"

#include "engine/world/biome/biome_classifier.hpp"
#include "engine/world/universe/sdr_noise.hpp"

namespace gw::world::streaming {
namespace {

using gw::universe::hec_derive;
using gw::universe::HecDomain;
using gw::universe::SdrNoise;
using gw::universe::SdrParams;
using gw::universe::UniverseSeed;

[[nodiscard]] const SdrParams& terrain_sdr_params() noexcept {
    static constexpr SdrParams k{.octaves         = 4,
                               .base_frequency  = 0.048,
                               .lacunarity      = 2.0,
                               .persistence     = 0.46F,
                               .orientation_angle = 0.0F,
                               .anisotropy      = 1.0F};
    return k;
}

} // namespace

void fill_heightmap_row_range(HeightmapChunk&              hc,
                              const UniverseSeed&         chunk_seed,
                              const int                   y_begin,
                              const int                   y_end) noexcept {
    const SdrNoise height_noise(hec_derive(chunk_seed, HecDomain::Feature, 401, 0, 0), terrain_sdr_params());
    const gw::world::biome::BiomeClassifier              bio(chunk_seed);

    const Vec3_f64 origin = chunk_origin(hc.coord);
    constexpr double       cell = CHUNK_SIZE_METERS / static_cast<double>(kHeightmapResolution);
    const double wz    = origin.z() + (static_cast<double>(kHeightmapResolution) * 0.5 + 0.5) * cell;

    for (int y = y_begin; y < y_end; ++y) {
        for (int x = 0; x < static_cast<int>(kHeightmapResolution); ++x) {
            const double   wx   = origin.x() + (static_cast<double>(x) + 0.5) * cell;
            const double   wy   = origin.y() + (static_cast<double>(y) + 0.5) * cell;
            const Vec3_f64 wpos(wx, wy, wz);
            const std::size_t idx = static_cast<std::size_t>(y * static_cast<int>(kHeightmapResolution) + x);
            const float h = height_noise.sample(wx, wy, wz) * 40.0F + 48.0F;
            if (idx < kHeightmapSampleCount) {
                hc.heights[idx]   = h;
                hc.biome_ids[idx] = static_cast<std::uint8_t>(bio.classify(wpos));
            }
        }
    }
}

} // namespace gw::world::streaming
