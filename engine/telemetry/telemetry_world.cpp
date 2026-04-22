// engine/telemetry/telemetry_world.cpp

#include "engine/telemetry/telemetry_world.hpp"
#include "engine/telemetry/event_pipeline.hpp"
#include "engine/persist/local_store.hpp"
#include "engine/core/config/cvar_registry.hpp"

namespace gw::telemetry {

struct TelemetryWorld::Impl {
    TelemetryConfig             cfg{};
    gw::persist::ILocalStore*   store{nullptr};
    gw::config::CVarRegistry*   cvars{nullptr};
    ConsentTier                 tier{ConsentTier::None};
    bool                        inited{false};
    double                      flush_accum{0};
};

TelemetryWorld::TelemetryWorld() : impl_(std::make_unique<Impl>()) {}
TelemetryWorld::~TelemetryWorld() { shutdown(); }

bool TelemetryWorld::initialize(TelemetryConfig cfg,
                                  gw::persist::ILocalStore* store,
                                  gw::config::CVarRegistry*  cvars) {
    shutdown();
    impl_->cfg    = std::move(cfg);
    impl_->store  = store;
    impl_->cvars  = cvars;
    if (impl_->store && impl_->cvars) impl_->tier = consent_load(*impl_->store);
    impl_->inited = (impl_->store != nullptr && impl_->cvars != nullptr);
    return impl_->inited;
}

void TelemetryWorld::shutdown() {
    impl_->store  = nullptr;
    impl_->cvars  = nullptr;
    impl_->inited = false;
}

bool TelemetryWorld::initialized() const noexcept { return impl_->inited; }

void TelemetryWorld::set_consent_tier(ConsentTier t) noexcept { impl_->tier = t; }

ConsentTier TelemetryWorld::consent_tier() const noexcept { return impl_->tier; }

TelemetryError TelemetryWorld::record_event(std::string_view name, std::string_view props_json) {
    if (!impl_->inited) return TelemetryError::Disabled;
    return pipeline_record(*impl_->store, *impl_->cvars, impl_->tier, name, props_json);
}

TelemetryError TelemetryWorld::flush() {
    if (!impl_->inited) return TelemetryError::Disabled;
    return pipeline_flush_queue(*impl_->store, *impl_->cvars);
}

void TelemetryWorld::step(double dt_seconds) {
    if (!impl_->inited || !impl_->cvars) return;
    impl_->flush_accum += dt_seconds;
    const int interval = impl_->cvars->get_i32_or("tele.events.flush_interval_s", 60);
    if (interval > 0 && impl_->flush_accum >= static_cast<double>(interval)) {
        impl_->flush_accum = 0;
        (void)flush();
    }
}

} // namespace gw::telemetry
