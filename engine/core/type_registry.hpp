#pragma once

#include <cstdint>
#include <cstddef>
#include <typeinfo>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gw {
namespace core {

// Type registry for compile-time type information
class TypeRegistry {
public:
    // Register a type with unique ID
    template<typename T>
    static uint32_t register_type() {
        static uint32_t next_id = 1;
        return next_id++;
    }
    
    // Get type ID by type
    template<typename T>
    [[nodiscard]] static uint32_t get_type_id() {
        // Use RTTI with compile-time registration
        return typeid(T).hash_code();
    }
    
    // Get type name by ID
    [[nodiscard]] static std::string_view get_type_name(uint32_t type_id);
    
    // Check if type is registered
    template<typename T>
    [[nodiscard]] static bool is_registered() {
        return get_type_id<T>() != 0;
    }
    
private:
    static std::unordered_map<uint32_t, std::string_view> type_names_;
    static std::unordered_map<uint32_t, std::string_view> type_to_id_;
};

}  // namespace core
}  // namespace gw
