// engine/net/session.cpp — Phase 14 Wave 14J (ADR-0053).
//
// Dev-local session registry lives in-process so sandbox_netplay can spin
// up a server + two clients without any external service. Steam / EOS
// backends are stubbed; they return nullptr until their feature flags
// are wired.

#include "engine/net/session.hpp"

#include <mutex>
#include <unordered_map>

namespace gw::net {

namespace {

class DevLocalSessionProvider final : public ISessionProvider {
public:
    SessionError create(const SessionDesc& desc, SessionHandle& out_handle) override {
        std::lock_guard<std::mutex> g{mu_};
        if (desc.id.empty()) return SessionError::BackendFailed;
        auto it = by_id_.find(desc.id);
        if (it != by_id_.end()) return SessionError::AlreadyExists;
        SessionHandle h{};
        h.value = next_handle_++;
        SessionInfo info{};
        info.handle       = h;
        info.id           = desc.id;
        info.max_peers    = desc.max_peers;
        info.seed         = (desc.seed != 0) ? desc.seed : 0x9E3779B97F4A7C15ULL;
        info.backend_name = "dev-local";
        by_id_[desc.id]   = info;
        by_handle_[h.value] = info;
        out_handle        = h;
        return SessionError::None;
    }

    SessionError join(std::string_view id, SessionHandle& out_handle) override {
        std::lock_guard<std::mutex> g{mu_};
        auto it = by_id_.find(std::string{id});
        if (it == by_id_.end()) return SessionError::NotFound;
        if (it->second.peer_count >= it->second.max_peers) return SessionError::Full;
        it->second.peer_count += 1;
        by_handle_[it->second.handle.value] = it->second;
        out_handle = it->second.handle;
        return SessionError::None;
    }

    SessionError leave(SessionHandle h) override {
        std::lock_guard<std::mutex> g{mu_};
        auto it = by_handle_.find(h.value);
        if (it == by_handle_.end()) return SessionError::NotFound;
        if (it->second.peer_count > 0) it->second.peer_count -= 1;
        by_id_[it->second.id] = it->second;
        return SessionError::None;
    }

    std::size_t list(std::vector<SessionInfo>& out) const override {
        std::lock_guard<std::mutex> g{mu_};
        out.reserve(out.size() + by_id_.size());
        for (const auto& kv : by_id_) out.push_back(kv.second);
        return by_id_.size();
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return std::string_view{"dev-local"};
    }

private:
    mutable std::mutex                                  mu_{};
    std::uint64_t                                       next_handle_{1};
    std::unordered_map<std::string, SessionInfo>        by_id_{};
    std::unordered_map<std::uint64_t, SessionInfo>      by_handle_{};
};

} // namespace

std::unique_ptr<ISessionProvider> make_dev_local_session_provider() {
    return std::make_unique<DevLocalSessionProvider>();
}

std::unique_ptr<ISessionProvider> make_steam_session_provider() {
#if defined(GW_ENABLE_STEAM)
    // Real Steam backend hooks in here via the steamworks SDK. Kept
    // behind the define so external SDK headers stay quarantined.
    return nullptr;
#else
    return nullptr;
#endif
}

std::unique_ptr<ISessionProvider> make_eos_session_provider() {
#if defined(GW_ENABLE_EOS)
    return nullptr;
#else
    return nullptr;
#endif
}

} // namespace gw::net
