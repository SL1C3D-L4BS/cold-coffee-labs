// engine/world/planet/gptm_mesh_builder.cpp

#include "engine/world/planet/gptm_mesh_builder.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gw::world::planet {
namespace {

[[nodiscard]] constexpr float gptm_clamp_f(const float v, const float lo, const float hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

[[nodiscard]] constexpr int gptm_clamp_i(const int v, const int lo, const int hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

using gw::math::Vec3f;
using gw::world::streaming::CHUNK_SIZE_METERS;
using gw::world::streaming::HeightmapChunk;
using gw::world::streaming::chunk_origin;

[[nodiscard]] std::uint32_t lod_index(GptmLod lod) noexcept {
    return static_cast<std::uint32_t>(lod);
}

[[nodiscard]] float sample_height_grid(const HeightmapChunk& hc, const float grid_u,
                                       const float grid_v) noexcept {
    const float u = gptm_clamp_f(grid_u, 0.f, 31.999f);
    const float v = gptm_clamp_f(grid_v, 0.f, 31.999f);
    const int   i0 = static_cast<int>(std::floor(u));
    const int   j0 = static_cast<int>(std::floor(v));
    const int   i1 = std::min(i0 + 1, 31);
    const int   j1 = std::min(j0 + 1, 31);
    const float tx = u - static_cast<float>(i0);
    const float ty = v - static_cast<float>(j0);

    const auto h_at = [&](const int ix, const int iy) noexcept -> float {
        const std::size_t idx = static_cast<std::size_t>(iy * 32 + ix);
        if (idx >= gw::world::streaming::kHeightmapSampleCount) {
            return 0.f;
        }
        return hc.heights[idx];
    };

    const float h00 = h_at(i0, j0);
    const float h10 = h_at(i1, j0);
    const float h01 = h_at(i0, j1);
    const float h11 = h_at(i1, j1);
    const float hx0 = h00 * (1.f - tx) + h10 * tx;
    const float hx1 = h01 * (1.f - tx) + h11 * tx;
    return hx0 * (1.f - ty) + hx1 * ty;
}

[[nodiscard]] std::uint8_t sample_biome_nearest(const HeightmapChunk& hc, const int ix,
                                                const int iy) noexcept {
    const int       cx = gptm_clamp_i(ix, 0, 31);
    const int       cy = gptm_clamp_i(iy, 0, 31);
    const std::size_t idx = static_cast<std::size_t>(cy * 32 + cx);
    if (idx >= gw::world::streaming::kHeightmapSampleCount) {
        return 0;
    }
    return hc.biome_ids[idx];
}

[[nodiscard]] float meters_to_grid_u(const float px_m) noexcept {
    return px_m * 31.f / CHUNK_SIZE_METERS;
}

[[nodiscard]] Vec3f analytical_normal(const HeightmapChunk& hc, const float px_m,
                                      const float pz_m) noexcept {
    constexpr float kEpsGrid = 0.25f;
    const float     gu       = meters_to_grid_u(px_m);
    const float     gv       = meters_to_grid_u(pz_m);

    const float h_l = sample_height_grid(hc, gu - kEpsGrid, gv);
    const float h_r = sample_height_grid(hc, gu + kEpsGrid, gv);
    const float h_d = sample_height_grid(hc, gu, gv - kEpsGrid);
    const float h_u = sample_height_grid(hc, gu, gv + kEpsGrid);

    const float d_grid_u = 2.f * kEpsGrid;
    const float d_grid_v = 2.f * kEpsGrid;

    const float dhd_grid_u = (h_r - h_l) / d_grid_u;
    const float dhd_grid_v = (h_u - h_d) / d_grid_v;

    const float grid_to_m = CHUNK_SIZE_METERS / 31.f;
    const float dhdx      = dhd_grid_u / grid_to_m;
    const float dhdz      = dhd_grid_v / grid_to_m;

    Vec3f n(-dhdx, 1.f, -dhdz);
    const float len = std::sqrt(n.dot(n));
    if (len <= 1e-20f) {
        return Vec3f(0.f, 1.f, 0.f);
    }
    return Vec3f(-dhdx / len, 1.f / len, -dhdz / len);
}

void encode_oct_normal(const Vec3f& n, std::uint16_t& ox, std::uint16_t& oy) noexcept {
    const float denom = std::fabs(n.x()) + std::fabs(n.y()) + std::fabs(n.z());
    if (denom <= 1e-12f) {
        ox = 32768;
        oy = 32768;
        return;
    }
    float px = n.x() / denom;
    float py = n.y() / denom;
    const float nz = n.z();
    if (nz < 0.f) {
        px = (n.x() >= 0.f ? 1.f : -1.f) - px;
        py = (n.y() >= 0.f ? 1.f : -1.f) - py;
    }
    const auto pack = [](float s) noexcept -> std::uint16_t {
        const float t = gptm_clamp_f(s, -1.f, 1.f);
        return static_cast<std::uint16_t>(std::lround((t * 0.5f + 0.5f) * 65535.f));
    };
    ox = pack(px);
    oy = pack(py);
}

} // namespace

std::uint32_t GptmMeshBuilder::lod_cells(const GptmLod lod) noexcept {
    const std::uint32_t s = 32u >> lod_index(lod);
    return s == 0u ? 1u : s;
}

std::uint32_t GptmMeshBuilder::lod_vertex_count(const GptmLod lod) noexcept {
    const std::uint32_t c = lod_cells(lod);
    return c * c * 4u;
}

std::uint32_t GptmMeshBuilder::lod_index_count(const GptmLod lod) noexcept {
    const std::uint32_t c = lod_cells(lod);
    return c * c * 6u + 4u * c * 3u;
}

GptmMeshCpuData GptmMeshBuilder::build_lod(const HeightmapChunk& chunk, const GptmLod lod,
                                           IAllocator& arena) noexcept {
    const std::uint32_t cells = lod_cells(lod);
    const float         cell  = CHUNK_SIZE_METERS / static_cast<float>(cells);

    const std::uint32_t vcount = lod_vertex_count(lod);
    const std::uint32_t icount = lod_index_count(lod);

    void* vmem =
        arena.allocate(static_cast<std::size_t>(vcount) * sizeof(GptmVertex), alignof(GptmVertex));
    void* imem = arena.allocate(static_cast<std::size_t>(icount) * sizeof(std::uint32_t),
                                 alignof(std::uint32_t));
    if (vmem == nullptr || imem == nullptr) {
        return {};
    }

    auto* const verts = static_cast<GptmVertex*>(vmem);
    auto* const idx   = static_cast<std::uint32_t*>(imem);

    const gw::world::streaming::Vec3_f64 origin = chunk_origin(chunk.coord);

    Aabb3_f64 bounds{};
    bounds.min = gw::world::streaming::Vec3_f64(1e300, 1e300, 1e300);
    bounds.max = gw::world::streaming::Vec3_f64(-1e300, -1e300, -1e300);

    std::uint32_t vbase = 0;
    for (std::uint32_t qj = 0; qj < cells; ++qj) {
        for (std::uint32_t qi = 0; qi < cells; ++qi) {
            const float x0 = static_cast<float>(qi) * cell;
            const float x1 = static_cast<float>(qi + 1u) * cell;
            const float z0 = static_cast<float>(qj) * cell;
            const float z1 = static_cast<float>(qj + 1u) * cell;

            const float gu00 = meters_to_grid_u(x0);
            const float gu10 = meters_to_grid_u(x1);
            const float gu01 = meters_to_grid_u(x0);
            const float gu11 = meters_to_grid_u(x1);

            const float gv00 = meters_to_grid_u(z0);
            const float gv10 = meters_to_grid_u(z0);
            const float gv01 = meters_to_grid_u(z1);
            const float gv11 = meters_to_grid_u(z1);

            const float h00 = sample_height_grid(chunk, gu00, gv00);
            const float h10 = sample_height_grid(chunk, gu10, gv10);
            const float h01 = sample_height_grid(chunk, gu01, gv01);
            const float h11 = sample_height_grid(chunk, gu11, gv11);

            const int biome_ix =
                static_cast<int>(std::lround(meters_to_grid_u((x0 + x1) * 0.5f)));
            const int biome_iy =
                static_cast<int>(std::lround(meters_to_grid_u((z0 + z1) * 0.5f)));
            const std::uint8_t biome =
                sample_biome_nearest(chunk, biome_ix, biome_iy);

            const auto emit_corner = [&](const float px, const float pz, const float world_h,
                                         const float u, const float v) {
                const Vec3f n = analytical_normal(chunk, px, pz);
                std::uint16_t ox{};
                std::uint16_t oy{};
                encode_oct_normal(n, ox, oy);

                GptmVertex& vtx = verts[vbase];
                vtx.local_pos[0] = px;
                vtx.local_pos[1] = world_h - static_cast<float>(origin.y());
                vtx.local_pos[2] = pz;
                vtx.n_oct_x = ox;
                vtx.n_oct_y = oy;
                vtx.uv[0]   = u;
                vtx.uv[1]   = v;
                vtx.biome_id = biome;
                vtx._pad[0] = vtx._pad[1] = vtx._pad[2] = 0;
                std::memset(vtx._stride_tail, 0, sizeof(vtx._stride_tail));

                const gw::world::streaming::Vec3_f64 wp(
                    origin.x() + static_cast<double>(vtx.local_pos[0]),
                    origin.y() + static_cast<double>(vtx.local_pos[1]),
                    origin.z() + static_cast<double>(vtx.local_pos[2]));

                bounds.min = gw::world::streaming::Vec3_f64(
                    std::min(bounds.min.x(), wp.x()), std::min(bounds.min.y(), wp.y()),
                    std::min(bounds.min.z(), wp.z()));
                bounds.max = gw::world::streaming::Vec3_f64(
                    std::max(bounds.max.x(), wp.x()), std::max(bounds.max.y(), wp.y()),
                    std::max(bounds.max.z(), wp.z()));

                ++vbase;
            };

            emit_corner(x0, z0, h00, static_cast<float>(qi) / static_cast<float>(cells),
                        static_cast<float>(qj) / static_cast<float>(cells));
            emit_corner(x1, z0, h10, static_cast<float>(qi + 1u) / static_cast<float>(cells),
                        static_cast<float>(qj) / static_cast<float>(cells));
            emit_corner(x1, z1, h11, static_cast<float>(qi + 1u) / static_cast<float>(cells),
                        static_cast<float>(qj + 1u) / static_cast<float>(cells));
            emit_corner(x0, z1, h01, static_cast<float>(qi) / static_cast<float>(cells),
                        static_cast<float>(qj + 1u) / static_cast<float>(cells));
        }
    }

    std::uint32_t ib = 0;
    for (std::uint32_t qj = 0; qj < cells; ++qj) {
        for (std::uint32_t qi = 0; qi < cells; ++qi) {
            const std::uint32_t base = (qj * cells + qi) * 4u;
            idx[ib++] = base + 0u;
            idx[ib++] = base + 1u;
            idx[ib++] = base + 2u;
            idx[ib++] = base + 0u;
            idx[ib++] = base + 2u;
            idx[ib++] = base + 3u;
        }
    }

    // Four border strips — zero-area triangles that stitch skirt strips without extra vertices.
    for (std::uint32_t qj = 0; qj < cells; ++qj) {
        const std::uint32_t base = (qj * cells + 0u) * 4u;
        idx[ib++] = base + 0u;
        idx[ib++] = base + 3u;
        idx[ib++] = base + 0u;
    }
    for (std::uint32_t qj = 0; qj < cells; ++qj) {
        const std::uint32_t base = (qj * cells + (cells - 1u)) * 4u;
        idx[ib++] = base + 1u;
        idx[ib++] = base + 2u;
        idx[ib++] = base + 1u;
    }
    for (std::uint32_t qi = 0; qi < cells; ++qi) {
        const std::uint32_t base = (0u * cells + qi) * 4u;
        idx[ib++] = base + 0u;
        idx[ib++] = base + 1u;
        idx[ib++] = base + 0u;
    }
    for (std::uint32_t qi = 0; qi < cells; ++qi) {
        const std::uint32_t base = ((cells - 1u) * cells + qi) * 4u;
        idx[ib++] = base + 3u;
        idx[ib++] = base + 2u;
        idx[ib++] = base + 3u;
    }

    return GptmMeshCpuData{verts, idx, vcount, icount, bounds};
}

} // namespace gw::world::planet
