# ADR 0053 — `ISessionProvider` + dev-local backend + Steam/EOS stubs

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Waves 14J + 14K)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (weeks 093–094), with backend fills deferred to Phase 16

## 1. Context

*Two Client Night* needs a discovery + rendezvous layer: a host advertises a session, clients find it and join. Phase 14 ships a dev-local backend (file-discovery) so the sandbox is self-sufficient; Phase 16 plugs Steam and EOS behind the same interface. The contract must be backend-stable from day one — Steam and EOS headers never leak through `session_provider.hpp`.

## 2. Decision

### 2.1 Public API

```cpp
enum class SessionError : std::uint8_t {
    Ok = 0,
    AlreadyHosting,
    NotFound,
    BackendDisabled,
    Forbidden,
    TransportError,
    InvalidConfig,
};

struct SessionDesc {
    std::string                  name{};           // human-facing
    std::uint32_t                max_peers{8};
    std::uint32_t                port{27015};
    std::uint64_t                seed{0};          // shared RNG seed; 0 = generate
    gw::Vector<std::string>      tags{};           // filters
};

struct SessionSummary {
    std::string       id{};
    std::string       name{};
    std::string       host{};       // IP literal or opaque backend token
    std::uint16_t     port{0};
    std::uint32_t     peer_count{0};
    std::uint32_t     max_peers{0};
    std::uint64_t     seed{0};
};

struct SessionHandle {
    std::uint64_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
};

class ISessionProvider {
public:
    virtual ~ISessionProvider() = default;
    virtual std::expected<SessionHandle, SessionError> create(const SessionDesc&) = 0;
    virtual std::expected<void,         SessionError> join  (const SessionSummary&, SessionHandle& out) = 0;
    virtual std::vector<SessionSummary>                list  () = 0;
    virtual void                                       leave(SessionHandle) = 0;
    virtual std::string_view                           backend_name() const = 0;
};

std::unique_ptr<ISessionProvider> make_devlocal_session_provider(std::filesystem::path root);
std::unique_ptr<ISessionProvider> make_steam_session_provider_stub();   // always BackendDisabled
std::unique_ptr<ISessionProvider> make_eos_session_provider_stub();     // always BackendDisabled
```

The Steam / EOS stubs exist purely to prove the interface is backend-stable; they return `SessionError::BackendDisabled` on every operation and will be filled in Phase 16 behind `GW_ENABLE_STEAMWORKS` / `GW_ENABLE_EOS`.

### 2.2 dev-local backend

Writes `session_<id>.toml` under `root` (default `%APPDATA%/greywater/sessions/` on Windows, `$XDG_DATA_HOME/greywater/sessions/` on Linux) with shape:

```toml
id      = "a3f29-cafe-babe"
name    = "Two-Client Night"
host    = "127.0.0.1"
port    = 27015
seed    = "0x9E3779B97F4A7C15"
max_peers = 8
peer_count = 1
tags    = ["dev", "sandbox"]
created_at = "2026-04-21T12:00:00Z"
```

`list()` scans the directory; stale files (older than 60 s without a heartbeat) are filtered. Clients pick up `host:port` and `seed` directly.

### 2.3 Shared seed

The `seed` in the session descriptor becomes the shared RNG seed for the session: `physics.rng`, `gameai.blackboard.rng`, every deterministic subsystem folds it in at `NetworkWorld::initialize`. Same seed + same inputs = identical simulation on every peer from tick 0 — the precondition for ADR-0054 lockstep.

### 2.4 Security (dev-local only)

dev-local has **no authentication**. It is developer-mode only and the backend name emits `"devlocal (INSECURE)"` via `backend_name()`; the runtime refuses to select dev-local when `GW_RELEASE_BUILD` is set.

## 3. Consequences

- Phase 14 closes *Two Client Night* without Steam or EOS; Phase 16 swaps in the real backends without a single caller-site change.
- Seed-to-session binding means lockstep peers don't need an extra "sync" message — they agree from frame 0.
- Stale session files are garbage-collected on list(), so dev-local can't accumulate leaks across unclean exits.

## 4. Alternatives considered

- **Join-code pools (like GGPO)** — rejected for dev-local; the file discovery matches the developer workflow (launch two sandboxes in one terminal, both see the session).
- **Separate discovery and rendezvous services** — rejected; dev-local is a discovery + rendezvous singleton.
- **Require Steam/EOS from day one** — rejected; we need CI green on clean-checkout without SDK mounts.
