#pragma once
// engine/anim/anim_types.hpp — Phase 13 Wave 13A (ADR-0039).
//
// Opaque handles + scalar types used by the animation facade. No Ozz
// headers leak through this file (non-negotiable #3 + ADR-0039 §2).

#include <compare>
#include <cstdint>

namespace gw::anim {

// -----------------------------------------------------------------------------
// Opaque handles — 32-bit generational IDs. A handle with id == 0 is null.
// -----------------------------------------------------------------------------
struct SkeletonHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const SkeletonHandle&) const = default;
};

struct ClipHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const ClipHandle&) const = default;
};

struct GraphHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const GraphHandle&) const = default;
};

struct InstanceHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const InstanceHandle&) const = default;
};

struct MorphSetHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const MorphSetHandle&) const = default;
};

struct RagdollHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const RagdollHandle&) const = default;
};

// -----------------------------------------------------------------------------
// ECS association — matches gw::physics::EntityId by value so the bridges
// don't require conversion.
// -----------------------------------------------------------------------------
using EntityId = std::uint64_t;
inline constexpr EntityId kEntityNone = 0;

// -----------------------------------------------------------------------------
// Limits. Matches ADR-0039 §3 budgets and ACL uniform decoder default grid.
// -----------------------------------------------------------------------------
inline constexpr std::uint16_t kMaxJoints      = 256;
inline constexpr std::uint16_t kMaxMorphTargets = 128;
inline constexpr std::uint16_t kMaxBlendInputs = 16;

} // namespace gw::anim
