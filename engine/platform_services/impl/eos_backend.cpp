// engine/platform_services/impl/eos_backend.cpp — ADR-0067.
//
// Same header-quarantine contract as steam_backend.cpp.

#include "engine/platform_services/platform_services.hpp"

#if GW_ENABLE_EOS
#  include <eos_sdk.h>
#endif

namespace gw::platform_services {

#if GW_ENABLE_EOS

namespace {

class EosBackend final : public IPlatformServices {
public:
    EosBackend() = default;
    ~EosBackend() override { shutdown(); }

    bool initialize(const PlatformServicesConfig& cfg) override {
        cfg_ = cfg;
        if (cfg.dry_run_sdk) {
            inited_ = true;
            return true;
        }
        EOS_InitializeOptions io{};
        io.ApiVersion   = EOS_INITIALIZE_API_LATEST;
        io.ProductName  = cfg.product_name.c_str();
        io.ProductVersion = "1.0";
        if (EOS_Initialize(&io) != EOS_EResult::EOS_Success) return false;
        inited_ = true;
        return true;
    }

    void shutdown() override {
        if (inited_ && !cfg_.dry_run_sdk) {
            EOS_Shutdown();
        }
        inited_ = false;
    }

    void step(double /*dt*/) override {
        if (!inited_ || cfg_.dry_run_sdk) return;
        // EOS_Platform_Tick(platform_);  // wire real handle in prod
    }

    [[nodiscard]] UserRef current_user() const override {
        UserRef u{};
        u.display_name = "EOS User";
        u.id_hash_hex  = "eos";
        return u;
    }
    [[nodiscard]] bool signed_in() const noexcept override { return false; }

    PlatformError unlock_achievement(std::string_view) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    [[nodiscard]] bool is_unlocked(std::string_view) const override { return false; }
    PlatformError clear_achievement(std::string_view) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    PlatformError set_stat_i32(std::string_view, std::int32_t) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    PlatformError set_stat_f32(std::string_view, float) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    [[nodiscard]] std::int32_t get_stat_i32(std::string_view, std::int32_t f) const override { return f; }
    [[nodiscard]] float        get_stat_f32(std::string_view, float f) const override { return f; }
    PlatformError store_stats() override { return PlatformError::Ok; }
    PlatformError submit_score(std::string_view, std::int64_t) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    PlatformError top_scores(std::string_view, std::int32_t, std::vector<LeaderboardRow>& out) override { out.clear(); return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    PlatformError set_rich_presence(std::string_view, std::string_view) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    PlatformError list_subscribed_content(std::vector<UgcItem>& out) override { out.clear(); return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    PlatformError download_content(std::string_view) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    PlatformError publish_event(std::string_view, std::string_view) override { return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled; }
    [[nodiscard]] std::string_view backend_name() const noexcept override { return "eos"; }
    [[nodiscard]] std::uint64_t    event_count() const noexcept override { return 0; }
    [[nodiscard]] bool             would_accept(std::string_view) const noexcept override { return inited_; }

private:
    PlatformServicesConfig cfg_{};
    bool                   inited_{false};
};

} // namespace

std::unique_ptr<IPlatformServices> make_eos_platform_services() {
    return std::make_unique<EosBackend>();
}

#endif // GW_ENABLE_EOS

} // namespace gw::platform_services
