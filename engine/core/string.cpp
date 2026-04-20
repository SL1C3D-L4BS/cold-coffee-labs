#include "engine/core/string.hpp"

#include <cstring>

namespace gw {
namespace core {

Utf8String::Utf8String(std::string_view value) {
    static_cast<void>(assign(value));
}

bool Utf8String::assign(std::string_view value) {
    if (!is_valid_utf8(value)) {
        return false;
    }
    size_ = value.size();
    if (value.size() <= kSsoCapacity) {
        heap_.clear();
        heap_active_ = false;
        std::memcpy(sso_.data(), value.data(), value.size());
        sso_[value.size()] = '\0';
        return true;
    }

    heap_active_ = true;
    heap_.assign(value.data(), value.size());
    return true;
}

std::string_view Utf8String::view() const noexcept {
    return heap_active_ ? std::string_view(heap_) : std::string_view(sso_.data(), size_);
}

const char* Utf8String::c_str() const noexcept {
    return heap_active_ ? heap_.c_str() : sso_.data();
}

bool Utf8String::is_valid_utf8(std::string_view value) noexcept {
    int remaining = 0;
    for (unsigned char byte : value) {
        if (remaining == 0) {
            if ((byte & 0x80) == 0) {
                continue;
            }
            if ((byte & 0xE0) == 0xC0) {
                remaining = 1;
                continue;
            }
            if ((byte & 0xF0) == 0xE0) {
                remaining = 2;
                continue;
            }
            if ((byte & 0xF8) == 0xF0) {
                remaining = 3;
                continue;
            }
            return false;
        }
        if ((byte & 0xC0) != 0x80) {
            return false;
        }
        --remaining;
    }
    return remaining == 0;
}

}  // namespace core
}  // namespace gw
