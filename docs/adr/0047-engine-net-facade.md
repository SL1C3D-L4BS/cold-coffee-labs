# ADR 0047 — `engine/net/` facade + transport contract

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14A)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** weeks 084–095 (Phase 14 *Two Client Night*)

## 1. Context

Phase 14 opens the *Two Client Night* milestone (`docs/05_ROADMAP_AND_MILESTONES.md` §210): a dedicated server + two clients run at 60 Hz under a 120 ms injected RTT for ten minutes with zero desyncs and a voice round-trip ≤ 150 ms on LAN. Every Phase-14 deliverable attaches to one central PIMPL facade — `gw::net::NetworkWorld` — and every subsystem (transport, replication, prediction, voice, session) sits behind pure interfaces so the backend can be swapped without recompiling callers.

Binding non-negotiables (CLAUDE.md):

- **#3 header quarantine** — `<steam/**>`, `<opus.h>`, `<phonon.h>`, or any other third-party network header must never appear in `engine/net/*.hpp`; they are allowed only inside `engine/net/impl/*.cpp` behind compile-time gates.
- **#5 RAII** — every backend primitive (transport, codec, spatializer, session handle) lives under `std::unique_ptr` or `gw::Ref<T>`; no raw `new`/`delete`.
- **#6 RAII lifecycle** — `NetworkWorld::initialize(cfg, bus, sched)` is the single entry; a half-constructed `NetworkWorld` is structurally impossible.
- **#9 no STL containers in the replication hot path** — packet queues, delta scratch, ack rings use `engine/core/` containers + `engine/memory/` arenas.
- **#10 no `std::thread` / `std::async`** — every network job goes through `engine/jobs/Scheduler`.
- **#17 `float64` world-space positions** — transforms on the wire are quantized against the receiver's floating-origin anchor; never send a raw world `f64`.

## 2. Decision

### 2.1 Public API — PIMPL facade

`engine/net/network_world.hpp` is the single public entry. No third-party network header is reachable through it.

```cpp
class NetworkWorld {
public:
    NetworkWorld();
    ~NetworkWorld();
    NetworkWorld(const NetworkWorld&) = delete;
    NetworkWorld& operator=(const NetworkWorld&) = delete;

    [[nodiscard]] bool initialize(const NetworkConfig& cfg,
                                  events::CrossSubsystemBus* bus = nullptr,
                                  jobs::Scheduler* scheduler = nullptr);
    void shutdown();

    [[nodiscard]] bool           listen(std::uint16_t port);
    [[nodiscard]] PeerId         connect(std::string_view host, std::uint16_t port);
    void                         disconnect(PeerId peer) noexcept;

    [[nodiscard]] std::uint32_t  step(float delta_time_s);
    void                         step_fixed();

    [[nodiscard]] BackendKind    backend() const noexcept;
    // Replication, prediction, voice, session accessors — see §2.3.
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

### 2.2 Backend gating

| Backend | Flag | Default | Notes |
|---|---|---|---|
| **Null loopback** (in-proc ring buffer, deterministic) | — | always linked | 2c+server sandbox + every unit test; zero allocations on the hot path after warm-up |
| GameNetworkingSockets v1.4.1 | `GW_ENABLE_GNS` → `GW_NET_GNS` | OFF for `dev-*`, ON for `net-*` | Reliable + unreliable UDP, encryption, STUN-style NAT punch |
| Opus 1.5.x | `GW_ENABLE_OPUS` → `GW_NET_OPUS` | OFF for `dev-*`, ON for `net-*` | 20 ms frames, 24 kbps CBR default |
| Steam Audio spatializer | reuses Phase 10 `GW_ENABLE_PHONON` | inherits | Voice bus runs through the same listener |

Clean-checkout `dev-*` CI builds the null backend and reaches every Phase-14 code path deterministically — no third-party network dep is required for green.

### 2.3 Sub-facets

`NetworkWorld` owns (and hands back const references to) five subsystems:

| Accessor | Type | Owner |
|---|---|---|
| `transport()` | `ITransport&` | ADR-0047 |
| `replication()` | `ReplicationSystem&` | ADR-0048 / ADR-0049 |
| `prediction()` | `PredictionSystem&` | ADR-0050 / ADR-0054 |
| `voice()` | `VoiceSystem&` | ADR-0052 |
| `session()` | `ISessionProvider&` | ADR-0053 |
| `harness()` | `NetHarness&` | ADR-0051 |

All are constructed inside `initialize` and destroyed at `shutdown`; none exposes a two-phase `init`/`shutdown` below the facade.

### 2.4 Tick clock

`NetworkWorld::step_fixed()` advances one network tick at `net.tick_hz` (default 60). The accumulator is driven by the engine tick (`runtime::Engine::tick`) — the same clock that drives physics and anim (ADR-0037 §3, ADR-0039 §3). Changing `net.tick_hz` triggers a warning when physics and net clocks diverge; they are permitted to diverge only inside the rollback window (ADR-0050).

### 2.5 Thread ownership

| Thread | Owns |
|---|---|
| Main | `NetworkWorld` lifetime + public API; drains all `InFrameQueue<Net*>` buses |
| Jobs pool — `sim` lane | Replication build; delta encode/decode; voice encode/decode batches |
| Jobs pool — `net_io` lane (new, created when GNS is linked) | GNS poll; packet receive demux |
| Jobs pool — `background` lane | Session discovery file scan; replay record flush |

Zero `std::thread`, zero `std::async`. The net_io lane is a named lane inside `jobs::Scheduler`, not a private thread.

## 3. Consequences

- All Phase-14 subsystems land under one PIMPL — the pattern that already proved out for physics (0031) and anim (0039).
- Netcode ECS replication framework (ADR-0048) ties directly into this facade — no side-channel registrations.
- Upgrading GNS is ADR-worthy; any pin bump re-runs the Phase-14 desync + cross-platform replay gates (ADR-0055 §CI).
- Phase 16 session backends (Steam, EOS) land through ADR-0053 stubs already introduced here.

## 4. Alternatives considered

- **ENet direct** — rejected; no built-in encryption, lacks STUN-style NAT punch baseline, and Valve's GNS already carries that stack behind a permissive license.
- **yojimbo** — rejected; single-author LTS risk, DTLS-style crypto path we don't want to inherit.
- **A separate `engine/netcode/` split from `engine/net/`** — rejected; replication and transport are always used together, and the split doubles the header quarantine burden.
- **One mega `net` header** — rejected; split by concern (see §2.3) so hot-path translation units include only what they use.
