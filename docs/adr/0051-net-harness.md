# ADR 0051 — Netcode harness: link-sim + desync detector

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14F)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 089)

## 1. Context

We cannot ship a netcode stack whose correctness we cannot reproduce. Every unit test, every integration run, every CI gate must exercise the replication + reliability + rollback stack under controllable network conditions with deterministic drop patterns. Desyncs must be detected within 200 ms of divergence and every detection must ship enough evidence to reproduce offline.

## 2. Decision

### 2.1 LinkSim — `ITransport` wrapper

```cpp
struct LinkProfile {
    float         latency_mean_ms{0.0f};
    float         latency_jitter_ms{0.0f};
    float         loss_pct{0.0f};       // 0..100
    float         duplicate_pct{0.0f};
    std::uint32_t reorder_window{0};
    std::uint64_t seed{0x9E3779B97F4A7C15ULL};
};

std::unique_ptr<ITransport> wrap_linksim(std::unique_ptr<ITransport> inner,
                                         LinkProfile profile);
```

LinkSim buffers each outbound packet with a scheduled delivery tick computed from `latency_mean_ms + rng_gauss(jitter_ms)`. Packets are dropped via seeded Bernoulli on `loss_pct`, duplicated via Bernoulli on `duplicate_pct`, and reordered within `reorder_window` via a priority queue keyed on delivery tick.

Deterministic property: same `LinkProfile::seed` + same send sequence + same tick clock = same drop/delay pattern across OSes.

### 2.2 Profile matrix

Every replication/reliability/rollback test runs under at least four profiles:

| Name | Latency / Jitter | Loss | Reorder |
|---|---|---|---|
| `clean` | 0 / 0 | 0 | 0 |
| `lan` | 60 / 5 | 0 | 0 |
| `wan` | 120 / 20 | 2 | 0 |
| `hostile` | 250 / 50 | 5 | 3 |

The sandbox exit-gate (`two_client_night_ctest`) uses `{120 ms, 2 %}` which is the *Two Client Night* contract.

### 2.3 Desync detector

```cpp
class DesyncDetector {
public:
    void push_hash(PeerId, Tick, std::uint64_t determinism_hash);
    [[nodiscard]] bool divergence_detected() const noexcept;
    void dump_last_window(std::filesystem::path) const;
    [[nodiscard]] std::uint32_t window_ticks() const noexcept;
};
```

The detector keeps a ring of the last `net.desync.window_ticks` (default 200 = ~3.3 s @ 60 Hz) hashes per peer + the server's own hash. On every push, peer hashes are compared against the server's hash for the same tick; the first mismatch sets `divergence_detected()` and (when `net.desync.auto_dump = true`) writes a `.desync.bin` diff dump containing:

- per-peer ring of `(tick, hash)` entries
- last 200 inbound `EntityDelta` payloads
- LinkProfile parameters
- `.gwreplay` v3 file reference (ADR-0055 §Replay)

The format matches ADR-0037's side channel so an engineer can replay the recorded trace offline.

### 2.4 Console commands

| Command | Effect |
|---|---|
| `net.linksim <rtt_ms> <loss_pct>` | Install LinkSim on the current transport |
| `net.linksim off` | Remove LinkSim |
| `net.desync.dump` | Force a diff dump even without divergence |

## 3. Consequences

- Cross-platform reproducibility is testable without third-party traffic-shaping tools.
- A failing CI run ships an artifact that an engineer can replay on their workstation under the same seed.
- The harness is cheap enough to leave on in development — `net.linksim off` is the default.

## 4. Alternatives considered

- **OS-level traffic control (tc, netem)** — rejected as CI-non-portable and non-deterministic.
- **Stochastic drops without seed** — rejected; irreproducibility kills the feedback loop.
- **Write desync dumps only on divergence** — rejected; `net.desync.dump` is useful for manual inspection of "looks fine but I'm suspicious" states.
