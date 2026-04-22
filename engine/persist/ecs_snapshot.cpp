// engine/persist/ecs_snapshot.cpp

#include "engine/persist/ecs_snapshot.hpp"
#include "engine/persist/integrity.hpp"

#include "engine/ecs/serialize.hpp"
#include "engine/ecs/world.hpp"

#include <cstring>

namespace gw::persist {

std::uint64_t ecs_blob_determinism_hash(std::span<const std::byte> ecs_blob) noexcept {
    return blake3_prefix_u64(ecs_blob);
}

std::vector<gwsave::ChunkPayload> build_chunk_grid_demo(const gw::ecs::World& world) {
    std::vector<std::uint8_t> raw = gw::ecs::save_world(world, gw::ecs::SnapshotMode::PieSnapshot);
    std::vector<std::byte>    blob(raw.size());
    std::memcpy(blob.data(), raw.data(), raw.size());

    std::vector<gwsave::ChunkPayload> out;
    out.reserve(9);
    for (int gy = -1; gy <= 1; ++gy) {
        for (int gx = -1; gx <= 1; ++gx) {
            gwsave::ChunkPayload c;
            c.cx = gx;
            c.cy = gy;
            c.cz = 0;
            if (gx == 0 && gy == 0) {
                c.bytes = std::move(blob);
            } else {
                const std::uint32_t zero = 0;
                c.bytes.resize(sizeof(zero));
                std::memcpy(c.bytes.data(), &zero, sizeof(zero));
            }
            out.push_back(std::move(c));
        }
    }
    return out;
}

std::vector<std::byte> centre_chunk_payload(const std::vector<gwsave::ChunkPayload>& chunks) {
    for (const auto& c : chunks) {
        if (c.cx == 0 && c.cy == 0 && c.cz == 0) return c.bytes;
    }
    return {};
}

} // namespace gw::persist
