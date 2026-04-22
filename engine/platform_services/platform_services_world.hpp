#pragma once
// engine/platform_services/platform_services_world.hpp — ADR-0065 PIMPL façade.

#include "engine/platform_services/events_platform.hpp"
#include "engine/platform_services/platform_services.hpp"
#include "engine/platform_services/platform_services_config.hpp"
#include "engine/platform_services/platform_services_types.hpp"

#include <memory>

namespace gw::config {
class CVarRegistry;
}
namespace gw::events {
class CrossSubsystemBus;
}
namespace gw::jobs {
class Scheduler;
}

namespace gw::platform_services {

class PlatformServicesWorld {
public:
    PlatformServicesWorld();
    ~PlatformServicesWorld();

    PlatformServicesWorld(const PlatformServicesWorld&)            = delete;
    PlatformServicesWorld& operator=(const PlatformServicesWorld&) = delete;

    [[nodiscard]] bool initialize(PlatformServicesConfig     cfg,
                                   gw::config::CVarRegistry*  cvars,
                                   gw::events::CrossSubsystemBus* bus = nullptr,
                                   gw::jobs::Scheduler*       scheduler = nullptr);
    void               shutdown();
    [[nodiscard]] bool initialized() const noexcept;

    void step(double dt_seconds);

    // Convenience forwarders (so games can call `world.achievements().unlock(...)`).
    PlatformError unlock_achievement(std::string_view id);
    [[nodiscard]] bool is_unlocked(std::string_view id) const;
    PlatformError submit_score(std::string_view board, std::int64_t v);
    PlatformError set_rich_presence(std::string_view key, std::string_view utf8);
    PlatformError publish_event(std::string_view name, std::string_view json);

    [[nodiscard]] IPlatformServices*                       backend()  noexcept;
    [[nodiscard]] const IPlatformServices*                 backend()  const noexcept;
    [[nodiscard]] const PlatformServicesConfig&            config()   const noexcept;
    [[nodiscard]] std::uint64_t                            step_count() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::platform_services
