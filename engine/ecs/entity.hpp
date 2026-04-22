#pragma once
// engine/ecs/entity.hpp
// ECS entity handle: 64-bit generational index.
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.2.
//
// Layout: bits[0..31] = index, bits[32..63] = generation.
// Generation 0 is reserved to mean "never used / destroyed", so a freshly
// created entity starts at generation 1. Entity{0} is the unambiguous null.

#include <cstdint>
#include <functional>
#include <limits>

namespace gw {
namespace ecs {

struct Entity {
    std::uint64_t bits = 0;

    [[nodiscard]] constexpr std::uint32_t index() const noexcept {
        return static_cast<std::uint32_t>(bits);
    }
    [[nodiscard]] constexpr std::uint32_t generation() const noexcept {
        return static_cast<std::uint32_t>(bits >> 32);
    }
    [[nodiscard]] constexpr bool is_null() const noexcept { return bits == 0; }

    [[nodiscard]] static constexpr Entity null() noexcept { return {0}; }
    [[nodiscard]] static constexpr Entity from_parts(std::uint32_t index,
                                                      std::uint32_t generation) noexcept {
        return {static_cast<std::uint64_t>(index) |
                (static_cast<std::uint64_t>(generation) << 32)};
    }

    [[nodiscard]] friend constexpr bool operator==(Entity, Entity) noexcept = default;
};

static_assert(sizeof(Entity) == 8, "Entity must be 8 bytes");

// Small, stable integer type used to identify component types within a live
// ComponentRegistry. NOT portable across builds — use ComponentTypeInfo.stable_hash
// for anything durable (serialization).
using ComponentTypeId = std::uint16_t;
inline constexpr ComponentTypeId kInvalidComponentTypeId =
    std::numeric_limits<ComponentTypeId>::max();

// Maximum supported component types per registry. Matches the width of the
// bitset we use for archetype-match queries.
inline constexpr std::size_t kMaxComponentTypes = 256;

} // namespace ecs
} // namespace gw

namespace std {
template <>
struct hash<gw::ecs::Entity> {
    [[nodiscard]] size_t operator()(gw::ecs::Entity e) const noexcept {
        return std::hash<std::uint64_t>{}(e.bits);
    }
};
} // namespace std
