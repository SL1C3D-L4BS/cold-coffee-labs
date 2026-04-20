#pragma once

#include "field_reflection.hpp"
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <stdexcept>

namespace gw {
namespace core {

// Serialization context for writing binary data
class SerializationContext {
public:
    SerializationContext(std::vector<uint8_t>& buffer);
    ~SerializationContext();
    
    // Write different data types
    template<typename T>
    void write(const T& value);
    
    void write_bytes(const std::span<const uint8_t> data);
    
    [[nodiscard]] std::span<const uint8_t> data() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t offset() const;
};

// Serialization utilities
template<typename T>
[[nodiscard]] std::vector<uint8_t> serialize(const T& object) {
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(T) * 100);  // Rough estimate
    
    SerializationContext ctx(buffer);
    serialize_fields(ctx, object);
    
    buffer.shrink_to_fit();
    return buffer;
}

// Deserialize from binary data
template<typename T>
[[nodiscard]] T deserialize(const std::span<const uint8_t> data) {
    if (data.size() < sizeof(T)) {
        throw std::runtime_error("Insufficient data for deserialization");
    }
    
    T object{};
    // Create a temporary vector for deserialization
    std::vector<uint8_t> temp_buffer(data.begin(), data.end());
    SerializationContext ctx(temp_buffer);
    deserialize_fields(ctx, object);
    
    return object;
}

}  // namespace core
}  // namespace gw
