#pragma once
// engine/platform_services/platform_services_types.hpp — ADR-0065.

#include <cstdint>
#include <string>
#include <string_view>

namespace gw::platform_services {

enum class PlatformError : std::uint8_t {
    Ok = 0,
    NotSignedIn,
    NotFound,
    RateLimited,
    BackendDisabled,
    IoError,
    InvalidArg,
};

[[nodiscard]] constexpr std::string_view to_string(PlatformError e) noexcept {
    switch (e) {
        case PlatformError::Ok:              return "ok";
        case PlatformError::NotSignedIn:     return "not_signed_in";
        case PlatformError::NotFound:        return "not_found";
        case PlatformError::RateLimited:     return "rate_limited";
        case PlatformError::BackendDisabled: return "backend_disabled";
        case PlatformError::IoError:         return "io_error";
        case PlatformError::InvalidArg:      return "invalid_arg";
    }
    return "unknown";
}

struct UserRef {
    std::string id_hash_hex{};     // BLAKE3-128 hex of provider+userid
    std::string display_name{};
    std::string locale_bcp47{"en-US"};
};

struct LeaderboardRow {
    std::int64_t score{0};
    std::int32_t rank{0};
    std::string  user_id_hash_hex{};
    std::string  display_name{};
};

struct UgcItem {
    std::string id_hex{};          // provider-native id, hex-packed
    std::string title{};
    std::string author_id_hash{};
    std::uint64_t size_bytes{0};
    std::uint64_t subscribed_unix_ms{0};
};

} // namespace gw::platform_services
