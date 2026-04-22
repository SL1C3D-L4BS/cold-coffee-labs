#pragma once

#include "engine/persist/gwsave_format.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace gw::ecs {
class World;
}

namespace gw::persist {

/// Determinism fingerprint for ECS blob (BLAKE3 prefix u64).
[[nodiscard]] std::uint64_t ecs_blob_determinism_hash(std::span<const std::byte> ecs_blob) noexcept;

/// Serialize full world and build a 3×3 chunk grid: centre holds full snapshot; others empty placeholders.
[[nodiscard]] std::vector<gwsave::ChunkPayload> build_chunk_grid_demo(const gw::ecs::World& world);

/// Merge centre chunk + neighbours for tests that only load the middle blob.
[[nodiscard]] std::vector<std::byte> centre_chunk_payload(const std::vector<gwsave::ChunkPayload>& chunks);

} // namespace gw::persist
