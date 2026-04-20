#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace gw {
namespace core {

class Utf8String final {
public:
    static constexpr std::size_t kSsoCapacity = 22;

    Utf8String() = default;
    explicit Utf8String(std::string_view value);

    [[nodiscard]] bool assign(std::string_view value);
    [[nodiscard]] std::string_view view() const noexcept;
    [[nodiscard]] const char* c_str() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool uses_sso() const noexcept { return !heap_active_; }
    [[nodiscard]] static bool is_valid_utf8(std::string_view value) noexcept;

private:
    std::array<char, kSsoCapacity + 1> sso_{};
    std::string heap_{};
    std::size_t size_{0};
    bool heap_active_{false};
};

}  // namespace gw
}  // namespace core
