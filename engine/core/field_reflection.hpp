#pragma once

#include "type_registry.hpp"
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace gw {
namespace core {

// Field descriptor for reflection
struct FieldDescriptor {
    uint32_t type_id;
    std::string_view name;
    std::size_t offset;
    std::size_t size;
};

// Field reference for accessing struct fields
template<typename T>
class FieldRef {
public:
    using value_type = typename T::value_type;
    
    FieldRef(T* instance, std::size_t offset)
        : instance_(instance), offset_(offset) {}
    
    [[nodiscard]] value_type& get() {
        return *reinterpret_cast<value_type*>(
            reinterpret_cast<uint8_t*>(instance_) + offset_
        );
    }
    
    [[nodiscard]] const value_type& get() const {
        return *reinterpret_cast<const value_type*>(
            reinterpret_cast<const uint8_t*>(instance_) + offset_
        );
    }
    
private:
    T* instance_;
    std::size_t offset_;
};

// Reflection utilities
template<typename T>
[[nodiscard]] constexpr auto get_fields() {
    if constexpr (std::is_class_v<T>) {
        return std::apply([](auto... fields) {
            return std::make_tuple(
                FieldDescriptor{
                    TypeRegistry::get_type_id<typename decltype(fields)::value_type>(),
                    fields.name,
                    offsetof(T, fields),
                    sizeof(typename decltype(fields)::value_type)
                }...
            );
        }, T{});
    } else {
        return std::tuple<>();
    }
}

}  // namespace core
}  // namespace gw
