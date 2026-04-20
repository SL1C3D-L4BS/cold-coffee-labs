#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace gw::core {

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

}  // namespace gw::core
