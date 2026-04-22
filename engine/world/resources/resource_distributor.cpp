#include "engine/world/resources/resource_distributor.hpp"

#include "engine/world/streaming/chunk_coord.hpp"
#include "engine/world/universe/hec.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace gw::world::resources {
namespace {

using gw::universe::hec_derive;
using gw::universe::hec_to_rng;
using gw::universe::HecDomain;
using gw::universe::Pcg64Rng;
using gw::world::streaming::CHUNK_SIZE_METERS;
using gw::world::streaming::chunk_origin;
using gw::world::streaming::HeightmapChunk;
using gw::world::streaming::kHeightmapResolution;
using gw::world::streaming::kHeightmapSampleCount;

struct Vec2d {
    double x{};
    double y{};
};

struct Slot {
    Vec2d         p{};
    std::uint32_t type{0u};
};

[[nodiscard]] float sample_height_bilinear(const HeightmapChunk& hc, const double px, const double py) noexcept {
    constexpr double  cell = CHUNK_SIZE_METERS / static_cast<double>(kHeightmapResolution);
    const double      fx   = px / cell - 0.5;
    const double      fy   = py / cell - 0.5;
    const int         resm = static_cast<int>(kHeightmapResolution) - 1;
    const int         x0   = static_cast<int>(std::floor(fx));
    const int         y0   = static_cast<int>(std::floor(fy));
    const int         cx0  = std::max(0, std::min(x0, resm));
    const int         cy0  = std::max(0, std::min(y0, resm));
    const int         cx1  = std::min(resm, cx0 + 1);
    const int         cy1  = std::min(resm, cy0 + 1);
    const double      tx   = std::min(1.0, std::max(0.0, fx - static_cast<double>(cx0)));
    const double      ty   = std::min(1.0, std::max(0.0, fy - static_cast<double>(cy0)));
    const std::size_t i00  = static_cast<std::size_t>(cy0 * static_cast<int>(kHeightmapResolution) + cx0);
    const std::size_t i10  = static_cast<std::size_t>(cy0 * static_cast<int>(kHeightmapResolution) + cx1);
    const std::size_t i01  = static_cast<std::size_t>(cy1 * static_cast<int>(kHeightmapResolution) + cx0);
    const std::size_t i11  = static_cast<std::size_t>(cy1 * static_cast<int>(kHeightmapResolution) + cx1);
    const double      a    = (1.0 - tx) * (1.0 - ty) * static_cast<double>(hc.heights[i00])
        + tx * (1.0 - ty) * static_cast<double>(hc.heights[i10]);
    const double b    = (1.0 - tx) * ty * static_cast<double>(hc.heights[i01]) + tx * ty * static_cast<double>(hc.heights[i11]);
    return static_cast<float>(a + b);
}

[[nodiscard]] int dominant_biome_id(const HeightmapChunk& hc) noexcept {
    std::uint32_t hist[16]{};
    constexpr int nbiome = 10; // 0..9
    for (std::size_t i = 0; i < kHeightmapSampleCount; ++i) {
        const int b = static_cast<int>(hc.biome_ids[i]);
        if (b >= 0 && b < nbiome) {
            ++hist[static_cast<std::size_t>(b)];
        }
    }
    int best  = 0;
    int bestv = -1;
    for (int b = 0; b < nbiome; ++b) {
        const int v = static_cast<int>(hist[static_cast<std::size_t>(b)]);
        if (v > bestv) {
            bestv = v;
            best  = b;
        }
    }
    return best;
}

[[nodiscard]] bool far_from_slots(const Vec2d& c, const Slot* const s, const int n, const double min_d) noexcept {
    const double m2 = min_d * min_d;
    for (int i = 0; i < n; ++i) {
        const double dx = c.x - s[static_cast<std::size_t>(i)].p.x;
        const double dy = c.y - s[static_cast<std::size_t>(i)].p.y;
        if (dx * dx + dy * dy < m2) {
            return false;
        }
    }
    return true;
}

} // namespace

ResourceNodeBatch ResourceDistributor::generate(const HeightmapChunk& chunk, const ResourceDensityMap& density) noexcept {
    ResourceNodeBatch batch{};
    if (chunk.heights.size() != kHeightmapSampleCount || chunk.biome_ids.size() != kHeightmapSampleCount) {
        return batch;
    }

    const int         bdom  = dominant_biome_id(chunk);
    const std::size_t bidx  = static_cast<std::size_t>(bdom);
    const double      w     = static_cast<double>(CHUNK_SIZE_METERS);
    const double      area  = w * w;
    const double      glob  = static_cast<double>(kPairwiseMinSeparationM);

    Slot slots[64]{};
    int  ns = 0;

    for (std::uint32_t ti = 1; ti < static_cast<std::uint32_t>(ResourceType::COUNT); ++ti) {
        const float d = bidx < density.density.size() ? density.density[bidx][static_cast<std::size_t>(ti)] : 0.0F;
        if (d <= 0.0F) {
            continue;
        }
        if (ns >= 64) {
            break;
        }
        const std::int64_t  rt  = static_cast<std::int64_t>(ti);
        const auto          s   = hec_derive(chunk.seed, HecDomain::Loot, rt, 0, 0);
        Pcg64Rng            rng = hec_to_rng(s);
        // Expected separation for this stratum; drives target count, not a relaxed acceptance (acceptance
        // always enforces the global 0.4m floor).
        const double r_type = glob * (1.5 / (0.15 + 2.0 * static_cast<double>(d)));
        int          n_cap  = 64 - ns;
        int n_target    = static_cast<int>(area * static_cast<double>(d) * 0.02 / (3.14159265359 * r_type * r_type));
        n_target          = (std::max)(1, n_target);
        n_target          = (std::min)({n_target, 16, n_cap});

        int  added  = 0;
        int  tries  = 0;
        while (added < n_target && ns < 64 && tries < 20000) {
            ++tries;
            const double u   = w * rng.next_f64_open();
            const double v   = w * rng.next_f64_open();
            const Vec2d  cand{u, v};
            if (!far_from_slots(cand, slots, ns, glob)) {
                continue;
            }
            slots[static_cast<std::size_t>(ns)].p    = cand;
            slots[static_cast<std::size_t>(ns)].type = ti;
            ++ns;
            ++added;
        }
    }

    if (ns == 0) {
        // Deterministic fallback: chunk centre in X/Y, still a valid *Sacrilege* shard site for CI.
        slots[0].p    = Vec2d{0.5 * w, 0.5 * w};
        slots[0].type = 1u;
        ns            = 1;
    }

    const auto origin = chunk_origin(chunk.coord);

    batch.count = static_cast<std::uint32_t>(ns);
    for (int i = 0; i < ns; ++i) {
        const Slot& s0 = slots[static_cast<std::size_t>(i)];
        const float  h  = sample_height_bilinear(chunk, s0.p.x, s0.p.y);
        const double wx = origin.x() + s0.p.x;
        const double wy = origin.y() + s0.p.y;
        const double wz = origin.z() + static_cast<double>(h);
        const auto   qseed = hec_derive(chunk.seed, HecDomain::Loot, static_cast<std::int64_t>(s0.type),
                                    static_cast<std::int64_t>(i) + 1, 0);
        Pcg64Rng  qr  = hec_to_rng(qseed);
        const float  qty  = 1.0F + 3.0F * qr.next_f32_open();
        ResourceNode& o = batch.nodes[static_cast<std::size_t>(i)];
        o.chunk         = chunk.coord;
        o.world_pos     = gw::world::streaming::Vec3_f64(wx, wy, wz);
        o.type          = s0.type;
        o.quantity      = qty;
        o.depleted      = false;
    }
    return batch;
}

} // namespace gw::world::resources
