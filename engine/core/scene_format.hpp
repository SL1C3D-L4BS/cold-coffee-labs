#pragma once

#include "serialization.hpp"
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace gw {
namespace core {

// Scene file format header
struct SceneHeader {
    static constexpr uint32_t MAGIC = 0x475747247u;
    static constexpr uint32_t VERSION = 1;
    static constexpr uint32_t HEADER_SIZE = 32;
    
    uint32_t magic;
    uint32_t version;
    uint32_t entity_count;
    uint32_t component_count;
    uint32_t object_count;
    uint32_t node_count;
    uint32_t light_count;
    uint32_t camera_count;
    uint32_t material_count;
    uint32_t texture_count;
    uint32_t animation_count;
};

// Scene format writer
class SceneWriter {
public:
    SceneWriter(std::vector<uint8_t>& buffer);
    ~SceneWriter();
    
    // Write scene header
    void write_header(const SceneHeader& header);
    
    // Write scene data
    template<typename T>
    void write_objects(const std::vector<T>& objects);
    
    [[nodiscard]] std::span<const uint8_t> data() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t offset() const;
};

// Scene format reader
class SceneReader {
public:
    SceneReader(const std::span<const uint8_t> data);
    ~SceneReader();
    
    // Read scene header
    [[nodiscard]] SceneHeader read_header();
    
    // Read scene data
    template<typename T>
    [[nodiscard]] std::vector<T> read_objects(uint32_t count);
    
    [[nodiscard]] bool validate() const;
};

}  // namespace core
}  // namespace gw
