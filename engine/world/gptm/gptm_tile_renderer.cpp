// engine/world/gptm/gptm_tile_renderer.cpp

#include "engine/world/gptm/gptm_tile_renderer.hpp"

#include "engine/ecs/world.hpp"
#include "engine/memory/frame_allocator.hpp"
#include "engine/render/frame_graph/command_buffer.hpp"
#include "engine/render/frame_graph/frame_graph.hpp"
#include "engine/render/frame_graph/types.hpp"

using gw::render::frame_graph::PassDesc;
using gw::render::frame_graph::QueueType;
#include "engine/world/gptm/gptm_lod_selector.hpp"

#include <cstring>
#include <string>

namespace gw::world::gptm {
namespace {

void split_vec3_f64(const gw::world::streaming::Vec3_f64& v, GptmTileInstance& out) noexcept {
    const auto split1 = [](const double d, float& lo, float& hi) noexcept {
        lo = static_cast<float>(d);
        hi = static_cast<float>(d - static_cast<double>(lo));
    };
    split1(v.x(), out.chunk_origin_x_lo, out.chunk_origin_x_hi);
    split1(v.y(), out.chunk_origin_y_lo, out.chunk_origin_y_hi);
    split1(v.z(), out.chunk_origin_z_lo, out.chunk_origin_z_hi);
}

} // namespace

void GptmTileRenderer::begin_frame(GptmMeshCache& cache, const gw::world::streaming::Vec3_f64& camera_world,
                                   const float fov_y_rad, const std::uint32_t screen_height_px,
                                   const gw::config::CVarRegistry* cvars) noexcept {
    for (auto& c : counts_) {
        c = 0;
    }
    total_instances_ = 0;

    for (const auto& kv : cache) {
        const gw::world::streaming::ChunkCoord coord = kv.first;
        const GptmLod lod =
            GptmLodSelector::select(coord, camera_world, fov_y_rad, screen_height_px, cvars);
        const std::size_t li = static_cast<std::size_t>(lod);
        if (counts_[li] >= kMaxTilesPerLod) {
            continue;
        }
        const GptmTile& t = kv.second[li];
        if (t.index_count == 0u || t.vertex_buffer.buffer == 0u || t.index_buffer.buffer == 0u) {
            continue;
        }
        const std::uint32_t id            = total_instances_++;
        instance_ids_[li][counts_[li]] = id;
        tiles_by_lod_[li][counts_[li]++] = &t;
    }
}

void GptmTileRenderer::tick(gw::ecs::World& world, const RenderView& view,
                             gw::render::frame_graph::FrameGraph& fg,
                             gw::memory::FrameAllocator&         frame_alloc) noexcept {
    (void)world;
    (void)view;

    if (fg.is_compiled()) {
        return;
    }

    GptmTileInstance* inst_cpu = nullptr;
    if (total_instances_ > 0u) {
        const std::size_t bytes =
            static_cast<std::size_t>(total_instances_) * sizeof(GptmTileInstance);
        void* const mem = frame_alloc.allocate(bytes, alignof(GptmTileInstance));
        inst_cpu        = static_cast<GptmTileInstance*>(mem);
        if (inst_cpu != nullptr) {
            std::memset(inst_cpu, 0, bytes);
            for (std::size_t li = 0; li < 5; ++li) {
                for (std::uint32_t i = 0; i < counts_[li]; ++i) {
                    const GptmTile* t = tiles_by_lod_[li][i];
                    if (t == nullptr) {
                        continue;
                    }
                    const gw::world::streaming::Vec3_f64 o = gw::world::streaming::chunk_origin(t->coord);
                    GptmTileInstance&                      dst = inst_cpu[instance_ids_[li][i]];
                    split_vec3_f64(o, dst);
                    dst.biome   = static_cast<std::uint32_t>(t->dominant_biome);
                    dst.padding = 0u;
                }
            }
        }
    }

    for (std::uint32_t L = 0; L < 5; ++L) {
        PassDesc pass(std::string("gptm_lod") + static_cast<char>('0' + static_cast<int>(L)));
        pass.queue = QueueType::Graphics;

        const std::uint32_t lod_capture = L;
        pass.execute =
            [this, lod_capture](gw::render::frame_graph::CommandBuffer& cmd) noexcept {
                for (std::uint32_t i = 0; i < counts_[lod_capture]; ++i) {
                    const GptmTile* t = tiles_by_lod_[lod_capture][i];
                    if (t == nullptr || t->index_count == 0u) {
                        continue;
                    }
                    const VkBuffer vb = reinterpret_cast<VkBuffer>(t->vertex_buffer.buffer);
                    const VkBuffer ib = reinterpret_cast<VkBuffer>(t->index_buffer.buffer);
                    cmd.bind_vertex_buffer(vb, 0);
                    cmd.bind_index_buffer(ib, 0, VK_INDEX_TYPE_UINT32);
                    cmd.draw_indexed(t->index_count, 1u, 0u, 0,
                                     instance_ids_[lod_capture][i]);
                }
            };

        [[maybe_unused]] const auto pass_result = fg.add_pass(std::move(pass));
        (void)pass_result;
    }

    (void)inst_cpu;
}

} // namespace gw::world::gptm
