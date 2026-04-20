#pragma once

#include "chunk.hpp"
#include "archetype.hpp"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <array>

namespace gw {
namespace ecs {

// System base class - all systems inherit from this
class System {
public:
    virtual ~System() = default;
    
    // Update all entities in chunk
    virtual void update(Chunk& chunk, float delta_time) = 0;
    
    // Render all entities in chunk
    virtual void render(Chunk& chunk) = 0;
    
    // Get system priority for execution order
    [[nodiscard]] virtual uint32_t priority() const = 0;
};

// System registry for managing all system types
template<typename... SystemTypes>
class SystemRegistry {
public:
    SystemRegistry() = default;
    virtual ~SystemRegistry() = default;
    
    // Register a system type
    template<typename T>
    void register_system() {
        static_assert(std::is_base_of_v<T, System>, "System must inherit from System");
        systems_[static_cast<size_t>(T::PRIORITY)] = std::make_unique<T>();
    }
    
    // Get system by type
    template<typename T>
    [[nodiscard]] T* get_system() const {
        static_assert(std::is_base_of_v<T, System>, "System must inherit from System");
        return static_cast<T*>(systems_[static_cast<size_t>(T::PRIORITY)].get());
    }
    
    // Update all systems in priority order
    void update_all(Chunk& chunk, float delta_time);
    
private:
    static constexpr std::size_t MAX_SYSTEMS = 16;
    std::array<std::unique_ptr<System>, MAX_SYSTEMS> systems_;
};

}  // namespace ecs
}  // namespace gw
