#pragma once
// engine/world/resources — numeric resource identity for procedural systems. Sacrilege market names
// (SinCrystal, …) live only here; gameplay/ may re-export the same display strings. Column width for
// density / packed IDs is `ResourceIdLimits::MAX_RESOURCES` (spec shorthand: the proof digest namespace
// documents what players share for a *Hell Seed*; those Murmur3 vectors live in
// `engine/world/universe/determinism_validator.hpp` → `gw::universe::GoldenHashes`).

#include "engine/core/array.hpp"
#include "engine/world/streaming/chunk_data.hpp"
#include "engine/world/streaming/chunk_coord.hpp"
#include <cstdint>

namespace gw::world::resources {

/// Opaque u32 in engine/world. Zero is the sentinel “no resource / None”.
using ResourceId = std::uint32_t;

struct ResourceIdLimits {
    /// Columns in `ResourceDensityMap::density` and upper bound (exclusive) for resource kinds.
    static constexpr ResourceId MAX_RESOURCES = 7u;
};

enum class ResourceType : std::uint32_t {
    None = 0,
    SinCrystal,
    MantraShard,
    BloodIron,
    SoulStone,
    HellGold,
    BoneDust,
    COUNT, ///< = 7 (None + 6)
};

struct ResourceNode {
    gw::world::streaming::ChunkCoord chunk{};
    gw::world::streaming::Vec3_f64   world_pos{};
    ResourceId                        type{0u};
    float                            quantity{0.0F};
    bool                              depleted{false};
};

struct ResourceNodeBatch {
    static constexpr std::uint32_t kMaxPerChunk = 64u;
    gw::core::Array<ResourceNode, 64> nodes{};
    std::uint32_t                       count{0};
};

/// f32[biome][resource kind], indexed by `static_cast<int>(biome) < BiomeId::COUNT` and `resource < MAX_RESOURCES`
/// (row 0 / None is unused; kept so indices align with `ResourceType` values).
struct ResourceDensityMap {
    // BiomeId::COUNT rows × ResourceIdLimits::MAX_RESOURCES resource slots (0 = None, unused).
    std::array<std::array<float, ResourceIdLimits::MAX_RESOURCES>, static_cast<std::size_t>(gw::world::streaming::BiomeId::COUNT)>
        density{};

    [[nodiscard]] static ResourceDensityMap make_uniform(float v) noexcept {
        ResourceDensityMap m{};
        for (auto& row : m.density) {
            row.fill(v);
        }
        return m;
    }
};

} // namespace gw::world::resources
