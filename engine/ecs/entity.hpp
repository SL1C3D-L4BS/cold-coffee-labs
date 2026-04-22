#pragma once
// engine/ecs/entity.hpp
// ECS entity handle: 64-bit generational index.
// See docs/10_APPENDIX_ADRS_AND_REFERENCES.md §2.2.
//
// Layout: bits_[0..31] = index, bits_[32..63] = generation.
// Generation 0 is reserved to mean "never used / destroyed", so a freshly
// created entity starts at generation 1. Default-constructed Entity and
// Entity::null() are the unambiguous null handle.

#include <cstdint>
#include <functional>
#include <limits>

namespace gw::ecs {

struct Entity {
private:
    std::uint64_t bits_ = 0;

public:
    [[nodiscard]] constexpr std::uint32_t index() const noexcept {
        return static_cast<std::uint32_t>(bits_);
    }
    [[nodiscard]] constexpr std::uint32_t generation() const noexcept {
        return static_cast<std::uint32_t>(bits_ >> 32);
    }
    [[nodiscard]] constexpr bool is_null() const noexcept { return bits_ == 0; }

    // Full 64-bit representation (serialization, hashing, editor C API).
    [[nodiscard]] constexpr std::uint64_t raw_bits() const noexcept { return bits_; }

    [[nodiscard]] static constexpr Entity null() noexcept { return {}; }

    [[nodiscard]] static constexpr Entity from_parts(std::uint32_t index,
                                                     std::uint32_t generation) noexcept {
        return from_raw_bits(static_cast<std::uint64_t>(index) |
                             (static_cast<std::uint64_t>(generation) << 32));
    }

    // Reconstructs a handle from a stored 64-bit value (e.g. after load).
    [[nodiscard]] static constexpr Entity from_raw_bits(std::uint64_t raw_bits) noexcept {
        Entity entity{};
        entity.bits_ = raw_bits;
        return entity;
    }

    [[nodiscard]] constexpr bool operator==(const Entity& other) const noexcept = default;
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

} // namespace gw::ecs

namespace std {
template <>
struct hash<gw::ecs::Entity> {
    [[nodiscard]] size_t operator()(const gw::ecs::Entity& entity) const noexcept {
        return std::hash<std::uint64_t>{}(entity.raw_bits());
    }
};
} // namespace std
