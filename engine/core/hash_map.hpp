#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace gw {
namespace core {

struct StringHash {
    using is_transparent = void;

    [[nodiscard]] std::size_t operator()(std::string_view value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
    [[nodiscard]] std::size_t operator()(const std::string& value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
    [[nodiscard]] std::size_t operator()(const char* value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
};

template <typename Value>
using StringHashMap = std::unordered_map<std::string, Value, StringHash, std::equal_to<>>;

/// Engine-wide alias for `std::unordered_map` with explicit hash/equality (used by streaming, ECS, etc.).
template <typename Key,
          typename Value,
          typename Hash    = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using HashMap = std::unordered_map<Key, Value, Hash, KeyEqual>;

}  // namespace core
}  // namespace gw
