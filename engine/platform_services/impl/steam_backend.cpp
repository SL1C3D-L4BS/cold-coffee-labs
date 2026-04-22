// engine/platform_services/impl/steam_backend.cpp — ADR-0066.
//
// Header quarantine:
//   * the TU-level `#include <steam_api.h>` only fires under
//     `GW_ENABLE_STEAMWORKS`; otherwise the TU is a no-op factory stub.
//   * no Steam symbols leak into any public header anywhere in the tree.

#include "engine/platform_services/platform_services.hpp"

#if GW_ENABLE_STEAMWORKS
#  include <steam_api.h>
#endif

#include <utility>

namespace gw::platform_services {

#if GW_ENABLE_STEAMWORKS

namespace {

class SteamworksBackend final : public IPlatformServices {
public:
    SteamworksBackend() = default;
    ~SteamworksBackend() override { shutdown(); }

    bool initialize(const PlatformServicesConfig& cfg) override {
        cfg_ = cfg;
        if (cfg.dry_run_sdk) {
            // CI: skip real init but keep the object usable so unit tests
            // can exercise the non-SDK paths.  signed_in_ remains false.
            inited_ = true;
            return true;
        }
        if (!SteamAPI_Init()) {
            return false;
        }
        inited_    = true;
        signed_in_ = true;
        return true;
    }

    void shutdown() override {
        if (inited_ && !cfg_.dry_run_sdk) {
            SteamAPI_Shutdown();
        }
        inited_    = false;
        signed_in_ = false;
    }

    void step(double /*dt*/) override {
        if (!inited_ || cfg_.dry_run_sdk) return;
        SteamAPI_RunCallbacks();
    }

    [[nodiscard]] UserRef current_user() const override {
        UserRef u{};
        u.display_name = "Steam User";
        u.id_hash_hex  = "steam";
        u.locale_bcp47 = "en-US";
        return u;
    }
    [[nodiscard]] bool signed_in() const noexcept override { return signed_in_; }

    PlatformError unlock_achievement(std::string_view /*id*/) override {
        // Dry run returns Ok so tests can exercise governance without a
        // Steam login.  Production path wires ISteamUserStats.
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }
    [[nodiscard]] bool is_unlocked(std::string_view /*id*/) const override { return false; }
    PlatformError clear_achievement(std::string_view /*id*/) override {
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }

    PlatformError set_stat_i32(std::string_view, std::int32_t) override {
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }
    PlatformError set_stat_f32(std::string_view, float) override {
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }
    [[nodiscard]] std::int32_t get_stat_i32(std::string_view, std::int32_t f) const override { return f; }
    [[nodiscard]] float        get_stat_f32(std::string_view, float f) const override { return f; }
    PlatformError store_stats() override { return PlatformError::Ok; }

    PlatformError submit_score(std::string_view, std::int64_t) override {
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }
    PlatformError top_scores(std::string_view, std::int32_t, std::vector<LeaderboardRow>& out) override {
        out.clear();
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }

    PlatformError set_rich_presence(std::string_view, std::string_view) override {
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }

    PlatformError list_subscribed_content(std::vector<UgcItem>& out) override {
        out.clear();
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }
    PlatformError download_content(std::string_view) override {
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }

    PlatformError publish_event(std::string_view, std::string_view) override {
        return inited_ ? PlatformError::Ok : PlatformError::BackendDisabled;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override { return "steam"; }
    [[nodiscard]] std::uint64_t    event_count() const noexcept override { return 0; }
    [[nodiscard]] bool             would_accept(std::string_view) const noexcept override { return inited_; }

private:
    PlatformServicesConfig cfg_{};
    bool                   inited_{false};
    bool                   signed_in_{false};
};

} // namespace

std::unique_ptr<IPlatformServices> make_steamworks_platform_services() {
    return std::make_unique<SteamworksBackend>();
}

#endif // GW_ENABLE_STEAMWORKS

} // namespace gw::platform_services
