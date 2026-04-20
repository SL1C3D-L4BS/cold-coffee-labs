#pragma once

#include "entity.hpp"
#include "component.hpp"
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include <vector>

namespace gw {
namespace ecs {

// Forward declaration
class Archetype;

// Chunk stores entities and their components
// 32x32x32 meter chunks = 1,048,576 entities at full density
class Chunk {
public:
    static constexpr std::size_t SIZE = 32;
    static constexpr std::size_t HEIGHT = 32;
    static constexpr std::size_t WIDTH = 32;
    static constexpr std::size_t DEPTH = 32;
    
    using EntityIndex = uint16_t;
    using ComponentIndex = uint16_t;
    
    Chunk();
    ~Chunk();
    
    // Entity management
    [[nodiscard]] EntityHandle spawn_entity(const Archetype& archetype);
    void destroy_entity(EntityHandle entity);
    [[nodiscard]] bool is_alive(EntityHandle entity) const;
    
    // Component access
    template<typename T>
    [[nodiscard]] T* get_component(EntityHandle entity);
    
    template<typename T>
    void add_component(EntityHandle entity, const T& component);
    
    template<typename T>
    void remove_component(EntityHandle entity);
    
    // System interface
    void update(float delta_time);
    void render();
    
    // Iterator for entities in chunk
    class Iterator {
    public:
        Iterator(Chunk& chunk);
        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] EntityHandle entity() const;
        [[nodiscard]] EntityIndex index() const;
        
        Iterator& operator++();
        [[nodiscard]] bool operator!=(const Iterator& other) const;
        [[nodiscard]] bool operator==(const Iterator& other) const;
        
    private:
        Chunk* chunk_;
        EntityIndex index_;
        EntityHandle current_entity_;
    };
    
    [[nodiscard]] Iterator begin() const;
    [[nodiscard]] Iterator end() const;
    
private:
    // Entity storage (sparse arrays for each component type)
    std::vector<EntityHandle> entities_;
    std::vector<bool> alive_;
    
    // Component storage (packed arrays for cache efficiency)
    std::vector<uint8_t> component_data_[static_cast<size_t>(ComponentType::MAX_COMPONENTS)];
    std::vector<ComponentIndex> component_indices_[static_cast<size_t>(ComponentType::MAX_COMPONENTS)];
    
    // Find free entity index
    [[nodiscard]] EntityIndex find_free_index() const;
    
    // Generation counter for entity handles
    uint32_t generation_{0};
};

}  // namespace ecs
}  // namespace gw
