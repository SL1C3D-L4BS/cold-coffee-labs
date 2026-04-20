#pragma once

#include <cstdint>
#include <cstddef>

namespace gw {
namespace ecs {

// Entity handle is a generational index to avoid ABA problems
// High bits are generation, low bits are index within generation
using EntityHandle = uint32_t;

constexpr EntityHandle INVALID_ENTITY = 0xFFFFFFFFu;

// Component type IDs - each component type gets a unique ID
enum class ComponentType : uint32_t {
    Transform = 0,
    Velocity = 1,
    Render = 2,
    Physics = 3,
    Animation = 4,
    Audio = 5,
    MAX_COMPONENTS
};

// Component flags for queries
enum class ComponentFlags : uint32_t {
    None = 0,
    Transform = 1u << static_cast<uint32_t>(ComponentType::Transform),
    Velocity = 1u << static_cast<uint32_t>(ComponentType::Velocity),
    Render = 1u << static_cast<uint32_t>(ComponentType::Render),
    Physics = 1u << static_cast<uint32_t>(ComponentType::Physics),
    Animation = 1u << static_cast<uint32_t>(ComponentType::Animation),
    Audio = 1u << static_cast<uint32_t>(ComponentType::Audio),
    All = (1u << static_cast<uint32_t>(ComponentType::MAX_COMPONENTS)) - 1u
};

}  // namespace ecs
}  // namespace gw
