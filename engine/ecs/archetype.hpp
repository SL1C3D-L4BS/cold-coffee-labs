#pragma once

#include "component.hpp"
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace gw {
namespace ecs {

// Archetype describes a combination of components
// Used for efficient entity creation and queries
class Archetype {
public:
    using mask_type = uint32_t;
    
    constexpr Archetype() noexcept : component_mask_(0) {}
    
    template<typename... ComponentTypes>
    constexpr Archetype(ComponentTypes... components) noexcept 
        : component_mask_(make_component_mask<ComponentTypes...>()) {}
    
    [[nodiscard]] mask_type component_mask() const noexcept { return component_mask_; }
    
    // Check if archetype contains specific component
    template<typename T>
    [[nodiscard]] constexpr bool has() const noexcept {
        return (component_mask_ & (1u << static_cast<uint32_t>(T::TYPE_ID))) != 0;
    }
    
private:
    template<typename... ComponentTypes>
    static constexpr uint32_t make_component_mask() noexcept {
        uint32_t mask = 0;
        ((mask |= (1u << static_cast<uint32_t>(ComponentTypes::TYPE_ID))), ...);
        return mask;
    }
    
    mask_type component_mask_;
};

}  // namespace ecs
}  // namespace gw
