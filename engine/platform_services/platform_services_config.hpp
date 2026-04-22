#pragma once
// engine/platform_services/platform_services_config.hpp — ADR-0065.

#include <cstdint>
#include <string>

namespace gw::platform_services {

struct PlatformServicesConfig {
    std::string  backend{"local"};       // "local" | "steam" | "eos"
    std::string  storage_dir{};           // expanded by the world ($user root)
    std::string  app_id{};                // Steam AppID / EOS ProductId
    std::string  product_name{"Greywater"};
    bool         dry_run_sdk{true};       // CI: never reach real SDK network
    std::int32_t rate_limit_per_minute{120};
    bool         achievements_enabled{true};
    bool         leaderboards_enabled{true};
    bool         rich_presence_enabled{true};
    bool         workshop_enabled{true};
};

} // namespace gw::platform_services
