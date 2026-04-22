#pragma once

#include "engine/telemetry/consent.hpp"
#include "engine/telemetry/telemetry_config.hpp"
#include "engine/telemetry/telemetry_types.hpp"

#include <memory>
#include <string_view>

namespace gw::config {
class CVarRegistry;
}
namespace gw::persist {
class ILocalStore;
}

namespace gw::telemetry {

class TelemetryWorld {
public:
    TelemetryWorld();
    ~TelemetryWorld();

    TelemetryWorld(const TelemetryWorld&)            = delete;
    TelemetryWorld& operator=(const TelemetryWorld&) = delete;

    [[nodiscard]] bool initialize(TelemetryConfig cfg,
                                  gw::persist::ILocalStore* store,
                                  gw::config::CVarRegistry*  cvars);
    void shutdown();
    [[nodiscard]] bool initialized() const noexcept;

    void set_consent_tier(ConsentTier t) noexcept;
    [[nodiscard]] ConsentTier consent_tier() const noexcept;

    [[nodiscard]] TelemetryError record_event(std::string_view name, std::string_view props_json);
    [[nodiscard]] TelemetryError flush();

    void step(double dt_seconds);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gw::telemetry
