// engine/platform_services/platform_cvars.cpp

#include "engine/platform_services/platform_cvars.hpp"

namespace gw::platform_services {

namespace {
constexpr std::uint32_t kP = config::kCVarPersist;
constexpr std::uint32_t kD = config::kCVarDevOnly;
} // namespace

PlatformCVars register_platform_cvars(config::CVarRegistry& r) {
    PlatformCVars c{};
    c.backend = r.register_string({
        "plat.backend", std::string{"local"}, kP, {}, {}, false,
        "Platform services backend: local | steam | eos.",
    });
    c.app_id = r.register_string({
        "plat.app_id", std::string{}, kP, {}, {}, false,
        "Steam AppID or EOS ProductId (ignored by local backend).",
    });
    c.achievements_enabled = r.register_bool({
        "plat.achievements.enabled", true, kP, {}, {}, false,
        "Gate achievement unlocks.",
    });
    c.leaderboards_enabled = r.register_bool({
        "plat.leaderboards.enabled", true, kP, {}, {}, false,
        "Gate leaderboard submit/query.",
    });
    c.rich_presence_enabled = r.register_bool({
        "plat.rich_presence.enabled", true, kP, {}, {}, false,
        "Gate rich-presence updates.",
    });
    c.workshop_enabled = r.register_bool({
        "plat.workshop.enabled", true, kP, {}, {}, false,
        "Gate Workshop / UGC download.",
    });
    c.rate_limit_per_minute = r.register_i32({
        "plat.rate_limit_per_minute", 120, kP, 1, 10000, true,
        "Per-event sliding-window rate cap (60 s).",
    });
    c.dry_run_sdk = r.register_bool({
        "plat.dry_run_sdk", true, kD, {}, {}, false,
        "CI: short-circuit real SDK calls even when Steam/EOS is compiled in.",
    });
    return c;
}

} // namespace gw::platform_services
