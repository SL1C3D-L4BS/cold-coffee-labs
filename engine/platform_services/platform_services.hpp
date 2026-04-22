#pragma once
// engine/platform_services/platform_services.hpp — IPlatformServices (ADR-0065).
//
// Pure-virtual façade. Includes only STL + engine types.  SDK headers
// (<steam_api.h>, <eos_sdk.h>, <accesskit.h>) must never appear in this file.

#include "engine/platform_services/events_platform.hpp"
#include "engine/platform_services/platform_services_config.hpp"
#include "engine/platform_services/platform_services_types.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::platform_services {

class IPlatformServices {
public:
    virtual ~IPlatformServices() = default;

    // Lifecycle.
    [[nodiscard]] virtual bool initialize(const PlatformServicesConfig& cfg) = 0;
    virtual void shutdown() = 0;
    virtual void step(double dt_seconds) = 0;

    // Identity.
    [[nodiscard]] virtual UserRef current_user() const = 0;
    [[nodiscard]] virtual bool signed_in() const noexcept = 0;

    // Achievements.
    virtual PlatformError unlock_achievement(std::string_view id) = 0;
    [[nodiscard]] virtual bool is_unlocked(std::string_view id) const = 0;
    virtual PlatformError clear_achievement(std::string_view id) = 0;

    // Stats.
    virtual PlatformError set_stat_i32(std::string_view key, std::int32_t v) = 0;
    virtual PlatformError set_stat_f32(std::string_view key, float v) = 0;
    [[nodiscard]] virtual std::int32_t get_stat_i32(std::string_view key,
                                                     std::int32_t fallback = 0) const = 0;
    [[nodiscard]] virtual float get_stat_f32(std::string_view key,
                                              float fallback = 0.f) const = 0;
    virtual PlatformError store_stats() = 0;

    // Leaderboards.
    virtual PlatformError submit_score(std::string_view board,
                                        std::int64_t     value) = 0;
    virtual PlatformError top_scores(std::string_view board,
                                      std::int32_t     k,
                                      std::vector<LeaderboardRow>& out) = 0;

    // Rich presence — value is UTF-8, already localized at the call-site.
    virtual PlatformError set_rich_presence(std::string_view key,
                                             std::string_view value_utf8) = 0;

    // Workshop / UGC.
    virtual PlatformError list_subscribed_content(std::vector<UgcItem>& out) = 0;
    virtual PlatformError download_content(std::string_view item_id_hex) = 0;

    // Custom analytics event (goes through rate-limiter).
    virtual PlatformError publish_event(std::string_view name,
                                         std::string_view payload_json) = 0;

    // Introspection.
    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t    event_count() const noexcept = 0;

    // Rate-limiter tap (testing).  Returns true if a call with this name
    // would succeed *right now* without incrementing the counter.
    [[nodiscard]] virtual bool would_accept(std::string_view name) const noexcept = 0;
};

// Factories.
[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_dev_local_platform_services();

[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_steamworks_platform_services();   // GW_ENABLE_STEAMWORKS=1 only

[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_eos_platform_services();          // GW_ENABLE_EOS=1 only

// Aggregated factory: picks real SDK when gated + available, else dev-local.
[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_platform_services_aggregated(std::string_view backend);

} // namespace gw::platform_services
