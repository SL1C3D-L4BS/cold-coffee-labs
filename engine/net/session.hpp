#pragma once
// engine/net/session.hpp — Phase 14 Wave 14J (ADR-0053).
//
// ISessionProvider abstracts session discovery / creation. The default
// backend is `dev-local` (in-memory registry); Steam and EOS backends are
// stubbed and compile out unless their feature flags are defined.

#include "engine/net/net_types.hpp"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gw::net {

enum class SessionError : std::uint8_t {
    None          = 0,
    AlreadyExists = 1,
    NotFound      = 2,
    BackendFailed = 3,
    Full          = 4,
};

struct SessionDesc {
    std::string   id{};
    std::uint32_t max_peers{4};
    std::uint64_t seed{0};
};

struct SessionInfo {
    SessionHandle handle{};
    std::string   id{};
    std::uint32_t peer_count{0};
    std::uint32_t max_peers{0};
    std::uint64_t seed{0};
    std::string   backend_name{};
};

class ISessionProvider {
public:
    virtual ~ISessionProvider() = default;

    ISessionProvider(const ISessionProvider&)            = delete;
    ISessionProvider& operator=(const ISessionProvider&) = delete;

    virtual SessionError   create(const SessionDesc& desc, SessionHandle& out_handle) = 0;
    virtual SessionError   join(std::string_view id, SessionHandle& out_handle) = 0;
    virtual SessionError   leave(SessionHandle h) = 0;
    virtual std::size_t    list(std::vector<SessionInfo>& out) const = 0;
    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;

protected:
    ISessionProvider() = default;
};

std::unique_ptr<ISessionProvider> make_dev_local_session_provider();
std::unique_ptr<ISessionProvider> make_steam_session_provider();  // returns null w/o GW_ENABLE_STEAM
std::unique_ptr<ISessionProvider> make_eos_session_provider();    // returns null w/o GW_ENABLE_EOS

} // namespace gw::net
