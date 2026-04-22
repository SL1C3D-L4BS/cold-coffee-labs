#pragma once
// engine/platform_services/events_platform.hpp — ADR-0065 §2.

#include "engine/platform_services/platform_services_types.hpp"

#include <cstdint>
#include <string>

namespace gw::platform_services {

struct PlatformSignInChanged {
    bool        signed_in{false};
    std::string backend_name{};
    std::string user_id_hash_hex{};
};

struct AchievementUnlocked {
    std::string backend_name{};
    std::string achievement_id{};
    std::uint64_t unix_ms{0};
};

struct RichPresenceChanged {
    std::string backend_name{};
    std::string key{};
    std::string value_utf8{};
};

struct WorkshopItemDownloaded {
    std::string backend_name{};
    std::string item_id_hex{};
    std::uint64_t size_bytes{0};
    PlatformError status{PlatformError::Ok};
};

} // namespace gw::platform_services
