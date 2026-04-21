#pragma once
// engine/physics/physics_types.hpp — Phase 12 Wave 12A (ADR-0031).
//
// Opaque typed handles + enums for the physics facade. No Jolt types leak
// into this header (non-negotiable #3).

#include <compare>
#include <cstdint>

namespace gw::physics {

// -----------------------------------------------------------------------------
// Opaque handles — 32-bit generational IDs. A handle with id == 0 is null.
// -----------------------------------------------------------------------------
struct BodyHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const BodyHandle&) const = default;
};

struct ShapeHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const ShapeHandle&) const = default;
};

struct ConstraintHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const ConstraintHandle&) const = default;
};

struct CharacterHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const CharacterHandle&) const = default;
};

struct VehicleHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const VehicleHandle&) const = default;
};

// -----------------------------------------------------------------------------
// Object + broadphase layers. ObjectLayer is the 4-bit slot we select from
// the 16-wide matrix. LayerMask is the bit set used by query filters.
// -----------------------------------------------------------------------------
enum class ObjectLayer : std::uint8_t {
    Static       = 0,
    Dynamic      = 1,
    Trigger      = 2,
    Character    = 3,
    Projectile   = 4,
    Debris       = 5,
    WaterVolume  = 6,
    UserA        = 7,
    UserB        = 8,
    // 9..15 reserved
    Count        = 16,
};

enum class BroadPhaseLayer : std::uint8_t {
    Static = 0,
    Moving = 1,
};

using LayerMask = std::uint16_t;
inline constexpr LayerMask kLayerMaskAll  = 0xFFFFu;
inline constexpr LayerMask kLayerMaskNone = 0x0000u;

[[nodiscard]] constexpr LayerMask layer_bit(ObjectLayer l) noexcept {
    return static_cast<LayerMask>(1u << static_cast<std::uint8_t>(l));
}

// -----------------------------------------------------------------------------
// Motion type. Matches the Jolt nomenclature so porting is one-to-one.
// -----------------------------------------------------------------------------
enum class BodyMotionType : std::uint8_t {
    Static    = 0,  // never moves; infinite mass
    Kinematic = 1,  // moved by user code; infinite mass
    Dynamic   = 2,  // moved by simulation
};

// -----------------------------------------------------------------------------
// ID entity association — whoever creates a body may attach an EntityId so
// contact / trigger / character events can route through the ECS without
// needing a side-table lookup.
// -----------------------------------------------------------------------------
using EntityId = std::uint64_t;
inline constexpr EntityId kEntityNone = 0;

// -----------------------------------------------------------------------------
// Origin enum for state-change bookkeeping (mirrors config::CVarOrigin).
// -----------------------------------------------------------------------------
enum class StateOrigin : std::uint8_t {
    Programmatic = 0,
    Input        = 1,
    Replay       = 2,
    Network      = 3,
};

} // namespace gw::physics
