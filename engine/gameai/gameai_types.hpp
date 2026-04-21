#pragma once
// engine/gameai/gameai_types.hpp — Phase 13 Wave 13E (ADR-0043, ADR-0044).
//
// Opaque handles + scalar types shared by the Behavior-Tree and Navmesh
// sub-facades. Neither Recast nor Detour headers leak through this file
// (non-negotiable #3 + ADR-0044 §2).

#include <compare>
#include <cstdint>

namespace gw::gameai {

struct BehaviorTreeHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const BehaviorTreeHandle&) const = default;
};

struct BehaviorInstanceHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const BehaviorInstanceHandle&) const = default;
};

struct NavmeshHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const NavmeshHandle&) const = default;
};

struct AgentHandle {
    std::uint32_t id{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr auto operator<=>(const AgentHandle&) const = default;
};

using EntityId = std::uint64_t;
inline constexpr EntityId kEntityNone = 0;

// Sentinel tile coords (no tile).
inline constexpr std::int32_t kInvalidTile = 0x7FFFFFFF;

// BT tick result.
enum class BTStatus : std::uint8_t {
    Invalid  = 0,
    Success  = 1,
    Failure  = 2,
    Running  = 3,
};

} // namespace gw::gameai
