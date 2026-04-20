#pragma once

#include "entity.hpp"
#include <cstdint>
#include <cstddef>
#include <functional>

namespace gw {
namespace ecs {

// Base component structure - all components inherit from this
struct Component {
    static constexpr uint32_t TYPE_ID = 0;
    
    virtual ~Component() = default;
    
    // Component data is stored in contiguous arrays for cache efficiency
    // Each component type defines its own data layout
};

// Component storage - chunk-based storage for components
template<typename ComponentType>
class ComponentStorage {
public:
    using value_type = ComponentType;
    using size_type = std::size_t;
    using reference = ComponentType&;
    
    ComponentStorage() = default;
    virtual ~ComponentStorage() = default;
    
    // Add a component to an entity
    virtual void add(EntityHandle entity, const ComponentType& component) = 0;
    
    // Get component reference for an entity
    virtual reference get(EntityHandle entity) = 0;
    
    // Remove component from entity
    virtual void remove(EntityHandle entity) = 0;
    
    // Iterate over all components
    template<typename Func>
    void for_each(Func&& func) const;
    
    // Clear all components
    virtual void clear() = 0;
    
    [[nodiscard]] virtual size_type size() const = 0;
    [[nodiscard]] virtual bool empty() const = 0;
};

}  // namespace ecs
}  // namespace gw
