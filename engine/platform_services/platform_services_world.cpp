// engine/platform_services/platform_services_world.cpp — ADR-0065.

#include "engine/platform_services/platform_services_world.hpp"

#include "engine/core/config/cvar_registry.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

namespace gw::platform_services {

namespace {

std::string expand_user_path(std::string s) {
    constexpr std::string_view kTok = "$user";
    const auto pos = s.find(kTok);
    if (pos == std::string::npos) return s;
    std::string base;
#ifdef _WIN32
    if (const char* p = std::getenv("LOCALAPPDATA")) base = p;
    else base = ".";
#else
    if (const char* p = std::getenv("XDG_DATA_HOME")) base = p;
    else if (const char* h = std::getenv("HOME")) base = std::string(h) + "/.local/share";
    else base = ".";
#endif
    base += "/CCL/Greywater";
    s.replace(pos, kTok.size(), base);
    return s;
}

} // namespace

struct PlatformServicesWorld::Impl {
    PlatformServicesConfig               cfg{};
    gw::config::CVarRegistry*            cvars{nullptr};
    gw::events::CrossSubsystemBus*      bus{nullptr};
    gw::jobs::Scheduler*                 sched{nullptr};
    std::unique_ptr<IPlatformServices> services{};
    bool                                 inited{false};
    std::uint64_t                        steps{0};
};

PlatformServicesWorld::PlatformServicesWorld() : impl_(std::make_unique<Impl>()) {}
PlatformServicesWorld::~PlatformServicesWorld() { shutdown(); }

bool PlatformServicesWorld::initialize(PlatformServicesConfig        cfg,
                                        gw::config::CVarRegistry*     cvars,
                                        gw::events::CrossSubsystemBus* bus,
                                        gw::jobs::Scheduler*          scheduler) {
    shutdown();
    impl_->cfg    = std::move(cfg);
    impl_->cvars  = cvars;
    impl_->bus    = bus;
    impl_->sched  = scheduler;

    if (impl_->cfg.storage_dir.empty()) {
        impl_->cfg.storage_dir = "$user/platform_services";
    }
    impl_->cfg.storage_dir = expand_user_path(impl_->cfg.storage_dir);
    std::error_code ec;
    std::filesystem::create_directories(impl_->cfg.storage_dir, ec);

    // Pull per-feature gates from CVars (last-write-wins vs cfg).
    if (impl_->cvars) {
        impl_->cfg.backend              = impl_->cvars->get_string_or("plat.backend", impl_->cfg.backend);
        impl_->cfg.app_id               = impl_->cvars->get_string_or("plat.app_id", impl_->cfg.app_id);
        impl_->cfg.achievements_enabled = impl_->cvars->get_bool_or("plat.achievements.enabled", impl_->cfg.achievements_enabled);
        impl_->cfg.leaderboards_enabled = impl_->cvars->get_bool_or("plat.leaderboards.enabled", impl_->cfg.leaderboards_enabled);
        impl_->cfg.rich_presence_enabled = impl_->cvars->get_bool_or("plat.rich_presence.enabled", impl_->cfg.rich_presence_enabled);
        impl_->cfg.workshop_enabled     = impl_->cvars->get_bool_or("plat.workshop.enabled", impl_->cfg.workshop_enabled);
        impl_->cfg.rate_limit_per_minute = impl_->cvars->get_i32_or("plat.rate_limit_per_minute", impl_->cfg.rate_limit_per_minute);
        impl_->cfg.dry_run_sdk          = impl_->cvars->get_bool_or("plat.dry_run_sdk", impl_->cfg.dry_run_sdk);
    }

    impl_->services = make_platform_services_aggregated(impl_->cfg.backend);
    if (!impl_->services || !impl_->services->initialize(impl_->cfg)) {
        impl_->services.reset();
        return false;
    }
    impl_->inited = true;
    return true;
}

void PlatformServicesWorld::shutdown() {
    if (impl_->services) impl_->services->shutdown();
    impl_->services.reset();
    impl_->inited = false;
}

bool PlatformServicesWorld::initialized() const noexcept { return impl_->inited; }

void PlatformServicesWorld::step(double dt) {
    if (!impl_->inited) return;
    ++impl_->steps;
    impl_->services->step(dt);
}

PlatformError PlatformServicesWorld::unlock_achievement(std::string_view id) {
    if (!impl_->inited) return PlatformError::BackendDisabled;
    return impl_->services->unlock_achievement(id);
}

bool PlatformServicesWorld::is_unlocked(std::string_view id) const {
    if (!impl_->inited) return false;
    return impl_->services->is_unlocked(id);
}

PlatformError PlatformServicesWorld::submit_score(std::string_view board, std::int64_t v) {
    if (!impl_->inited) return PlatformError::BackendDisabled;
    return impl_->services->submit_score(board, v);
}

PlatformError PlatformServicesWorld::set_rich_presence(std::string_view key,
                                                         std::string_view utf8) {
    if (!impl_->inited) return PlatformError::BackendDisabled;
    return impl_->services->set_rich_presence(key, utf8);
}

PlatformError PlatformServicesWorld::publish_event(std::string_view name,
                                                     std::string_view json) {
    if (!impl_->inited) return PlatformError::BackendDisabled;
    return impl_->services->publish_event(name, json);
}

IPlatformServices*       PlatformServicesWorld::backend()       noexcept { return impl_->services.get(); }
const IPlatformServices* PlatformServicesWorld::backend() const noexcept { return impl_->services.get(); }
const PlatformServicesConfig& PlatformServicesWorld::config() const noexcept { return impl_->cfg; }
std::uint64_t            PlatformServicesWorld::step_count() const noexcept { return impl_->steps; }

} // namespace gw::platform_services
