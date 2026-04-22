// engine/platform_services/dev_local_backend.cpp — ADR-0065 §2.
//
// In-memory `IPlatformServices` impl.  Persists nothing today; the
// `PlatformServicesWorld` is responsible for routing mutating calls into
// its per-user JSON file under `$user/platform_services/` when a storage
// dir is provided.  The dev-local backend itself is side-effect-free so
// unit tests can instantiate it freely.

#include "engine/platform_services/platform_services.hpp"
#include "engine/platform_services/rate_limiter.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <utility>

namespace gw::platform_services {

namespace {

class DevLocalPlatformServices final : public IPlatformServices {
public:
    DevLocalPlatformServices() = default;
    ~DevLocalPlatformServices() override { shutdown(); }

    bool initialize(const PlatformServicesConfig& cfg) override {
        cfg_ = cfg;
        rate_.set_per_minute(cfg.rate_limit_per_minute);
        inited_ = true;
        signed_in_ = true;
        user_ = UserRef{};
        user_.id_hash_hex = "00000000000000000000000000000000";
        user_.display_name = "Greywater Dev";
        user_.locale_bcp47 = "en-US";
        step_ms_ = 0;
        events_ = 0;
        return true;
    }

    void shutdown() override {
        achievements_.clear();
        stats_i32_.clear();
        stats_f32_.clear();
        leaderboards_.clear();
        rich_.clear();
        ugc_.clear();
        rate_.clear();
        inited_ = false;
        signed_in_ = false;
    }

    void step(double dt_seconds) override {
        step_ms_ += static_cast<std::uint64_t>(dt_seconds * 1000.0);
        rate_.evict(step_ms_);
    }

    [[nodiscard]] UserRef current_user() const override { return user_; }
    [[nodiscard]] bool    signed_in() const noexcept override { return signed_in_; }

    PlatformError unlock_achievement(std::string_view id) override {
        if (!inited_)                    return PlatformError::BackendDisabled;
        if (!cfg_.achievements_enabled)  return PlatformError::BackendDisabled;
        if (id.empty())                  return PlatformError::InvalidArg;
        if (!rate_.check(id, step_ms_))  return PlatformError::RateLimited;
        achievements_[std::string(id)] = true;
        ++events_;
        return PlatformError::Ok;
    }

    [[nodiscard]] bool is_unlocked(std::string_view id) const override {
        auto it = achievements_.find(std::string{id});
        return it != achievements_.end() && it->second;
    }

    PlatformError clear_achievement(std::string_view id) override {
        if (!inited_)                   return PlatformError::BackendDisabled;
        if (!cfg_.achievements_enabled) return PlatformError::BackendDisabled;
        if (id.empty())                 return PlatformError::InvalidArg;
        auto it = achievements_.find(std::string{id});
        if (it == achievements_.end())  return PlatformError::NotFound;
        it->second = false;
        return PlatformError::Ok;
    }

    PlatformError set_stat_i32(std::string_view key, std::int32_t v) override {
        if (!inited_) return PlatformError::BackendDisabled;
        if (key.empty()) return PlatformError::InvalidArg;
        stats_i32_[std::string{key}] = v;
        return PlatformError::Ok;
    }

    PlatformError set_stat_f32(std::string_view key, float v) override {
        if (!inited_) return PlatformError::BackendDisabled;
        if (key.empty()) return PlatformError::InvalidArg;
        stats_f32_[std::string{key}] = v;
        return PlatformError::Ok;
    }

    [[nodiscard]] std::int32_t get_stat_i32(std::string_view key,
                                              std::int32_t fallback) const override {
        auto it = stats_i32_.find(std::string{key});
        return it == stats_i32_.end() ? fallback : it->second;
    }

    [[nodiscard]] float get_stat_f32(std::string_view key,
                                       float fallback) const override {
        auto it = stats_f32_.find(std::string{key});
        return it == stats_f32_.end() ? fallback : it->second;
    }

    PlatformError store_stats() override {
        if (!inited_) return PlatformError::BackendDisabled;
        return PlatformError::Ok;
    }

    PlatformError submit_score(std::string_view board, std::int64_t value) override {
        if (!inited_)                    return PlatformError::BackendDisabled;
        if (!cfg_.leaderboards_enabled)  return PlatformError::BackendDisabled;
        if (board.empty())               return PlatformError::InvalidArg;
        if (!rate_.check(board, step_ms_)) return PlatformError::RateLimited;
        auto& rows = leaderboards_[std::string{board}];
        LeaderboardRow row{};
        row.score            = value;
        row.user_id_hash_hex = user_.id_hash_hex;
        row.display_name     = user_.display_name;
        rows.push_back(row);
        std::sort(rows.begin(), rows.end(),
                  [](const auto& a, const auto& b) { return a.score > b.score; });
        for (std::size_t i = 0; i < rows.size(); ++i) {
            rows[i].rank = static_cast<std::int32_t>(i + 1);
        }
        return PlatformError::Ok;
    }

    PlatformError top_scores(std::string_view board, std::int32_t k,
                              std::vector<LeaderboardRow>& out) override {
        out.clear();
        if (!inited_) return PlatformError::BackendDisabled;
        if (k <= 0)   return PlatformError::InvalidArg;
        auto it = leaderboards_.find(std::string{board});
        if (it == leaderboards_.end()) return PlatformError::NotFound;
        const auto& rows = it->second;
        const std::size_t n = std::min<std::size_t>(static_cast<std::size_t>(k), rows.size());
        out.insert(out.end(), rows.begin(), rows.begin() + static_cast<std::ptrdiff_t>(n));
        return PlatformError::Ok;
    }

    PlatformError set_rich_presence(std::string_view key,
                                     std::string_view value_utf8) override {
        if (!inited_)                     return PlatformError::BackendDisabled;
        if (!cfg_.rich_presence_enabled)  return PlatformError::BackendDisabled;
        if (key.empty())                  return PlatformError::InvalidArg;
        if (value_utf8.size() > 128)      return PlatformError::InvalidArg;
        rich_[std::string{key}] = std::string{value_utf8};
        return PlatformError::Ok;
    }

    PlatformError list_subscribed_content(std::vector<UgcItem>& out) override {
        out.clear();
        if (!inited_) return PlatformError::BackendDisabled;
        if (!cfg_.workshop_enabled) return PlatformError::BackendDisabled;
        out.insert(out.end(), ugc_.begin(), ugc_.end());
        return PlatformError::Ok;
    }

    PlatformError download_content(std::string_view item_id_hex) override {
        if (!inited_)               return PlatformError::BackendDisabled;
        if (!cfg_.workshop_enabled) return PlatformError::BackendDisabled;
        if (item_id_hex.empty())    return PlatformError::InvalidArg;
        // Dev-local: always succeed; simulate subscription.
        UgcItem it{};
        it.id_hex            = std::string{item_id_hex};
        it.title             = "dev-local-item";
        it.author_id_hash    = user_.id_hash_hex;
        it.subscribed_unix_ms = step_ms_;
        ugc_.push_back(it);
        return PlatformError::Ok;
    }

    PlatformError publish_event(std::string_view name,
                                 std::string_view /*payload_json*/) override {
        if (!inited_) return PlatformError::BackendDisabled;
        if (name.empty()) return PlatformError::InvalidArg;
        if (!rate_.check(name, step_ms_)) return PlatformError::RateLimited;
        ++events_;
        return PlatformError::Ok;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return "local";
    }

    [[nodiscard]] std::uint64_t event_count() const noexcept override {
        return events_;
    }

    [[nodiscard]] bool would_accept(std::string_view name) const noexcept override {
        return rate_.would_accept(name, step_ms_);
    }

private:
    PlatformServicesConfig cfg_{};
    UserRef                user_{};
    bool                   inited_{false};
    bool                   signed_in_{false};
    std::uint64_t          step_ms_{0};
    std::uint64_t          events_{0};
    RateLimiter            rate_{120};
    std::unordered_map<std::string, bool>                       achievements_{};
    std::unordered_map<std::string, std::int32_t>               stats_i32_{};
    std::unordered_map<std::string, float>                      stats_f32_{};
    std::unordered_map<std::string, std::vector<LeaderboardRow>> leaderboards_{};
    std::unordered_map<std::string, std::string>                rich_{};
    std::vector<UgcItem>                                        ugc_{};
};

} // namespace

std::unique_ptr<IPlatformServices> make_dev_local_platform_services() {
    return std::make_unique<DevLocalPlatformServices>();
}

std::unique_ptr<IPlatformServices>
make_platform_services_aggregated(std::string_view backend) {
    if (backend == "steam") {
#if GW_ENABLE_STEAMWORKS
        if (auto p = make_steamworks_platform_services()) return p;
#endif
    } else if (backend == "eos") {
#if GW_ENABLE_EOS
        if (auto p = make_eos_platform_services()) return p;
#endif
    }
    return make_dev_local_platform_services();
}

#if !GW_ENABLE_STEAMWORKS
std::unique_ptr<IPlatformServices> make_steamworks_platform_services() {
    return nullptr;
}
#endif

#if !GW_ENABLE_EOS
std::unique_ptr<IPlatformServices> make_eos_platform_services() {
    return nullptr;
}
#endif

} // namespace gw::platform_services
