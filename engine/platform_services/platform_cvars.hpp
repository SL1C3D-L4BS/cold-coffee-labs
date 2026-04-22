#pragma once
// engine/platform_services/platform_cvars.hpp — ADR-0065 §5 (7 plat.* CVars).

#include "engine/core/config/cvar_registry.hpp"

namespace gw::platform_services {

struct PlatformCVars {
    config::CVarRef<std::string>   backend;
    config::CVarRef<std::string>   app_id;
    config::CVarRef<bool>          achievements_enabled;
    config::CVarRef<bool>          leaderboards_enabled;
    config::CVarRef<bool>          rich_presence_enabled;
    config::CVarRef<bool>          workshop_enabled;
    config::CVarRef<std::int32_t>  rate_limit_per_minute;
    config::CVarRef<bool>          dry_run_sdk;
};

[[nodiscard]] PlatformCVars register_platform_cvars(config::CVarRegistry& r);

} // namespace gw::platform_services
