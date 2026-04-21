# ADR 0054 — Deterministic lockstep mode (GGPO-shaped)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 Wave 14K)
- **Deciders:** Cold Coffee Labs engine group
- **Scope:** Phase 14 (week 094), requires ADR-0037 + ADR-0048

## 1. Context

Client-server replication is the default path; lockstep is the second. Lockstep peers all run the same simulation off the same inputs and the wire carries only input packets — ideal for competitive 2–4 player matches where every byte matters and the simulation is already deterministic. GGPO's model (frame delay + rollback + SyncTest) is the proven shape.

## 2. Decision

### 2.1 API — `LockstepSession`

```cpp
class LockstepSession {
public:
    explicit LockstepSession(LockstepConfig cfg);

    void add_local_input(PeerId local, std::span<const std::byte> input);
    [[nodiscard]] InputSet synchronize_input();
    void advance_frame();

    // State save/load — bound to ADR-0037 determinism-hash region.
    void save_state(gw::Vector<std::byte>& out);
    void load_state(std::span<const std::byte> in);

    [[nodiscard]] SyncTestReport run_sync_test(std::uint32_t frames);

    [[nodiscard]] std::uint64_t determinism_hash(Tick) const noexcept;
};

struct LockstepConfig {
    std::uint32_t frame_delay{1};        // default GGPO frame delay
    std::uint32_t rollback_window{8};    // 133 ms @ 60 Hz
    std::uint32_t peer_count{2};
};
```

### 2.2 Frame-delay + rollback

Every local input arrives `frame_delay` ticks later on the wire; if a remote input is missing at `synchronize_input()` time, the session predicts from the last received (same-as-GGPO). When the real input arrives, the session rolls back to the prediction tick, rewrites the input, and re-advances. `save_state` is called by the rollback harness; `load_state` is called to restore.

### 2.3 SyncTest

`run_sync_test(N)` is the GGPO pattern: simulate N frames, roll back 1 frame, resimulate, assert `determinism_hash` at frame N is byte-identical pre- and post-rollback. It's CI-mandatory on Debug and runs on `two_client_night_ctest` setup.

### 2.4 Rollback-pure enforcement

Any code reached from a `save_state` ↔ `load_state` region — including physics, anim, BT — must be **pure of the wall clock**: no `std::chrono`, no `std::random_device`, no telemetry, no file I/O, no RNG that is not seeded from the frame seed. The enforcement is:

- a namespace macro `GW_ROLLBACK_PURE` annotates rollback-reachable code;
- a clang-tidy check rejects `std::chrono::*`, `std::random_device`, `std::this_thread::*`, `std::printf`, `std::fopen` under `GW_ROLLBACK_PURE`;
- the rollback test (`lockstep_rollback_purity_test`) runs SyncTest with `net.debug.rollback_paranoia=1`, which enables compile-time checks + a runtime tripwire for wall-clock reads.

This is the structural shield for K5 (determinism purity, `docs/12_ENGINEERING_PRINCIPLES.md`).

## 3. Consequences

- 2–4 peer matches run at the frame-per-packet efficiency lockstep implies.
- Rollback determinism is proven every commit (SyncTest runs in Debug every build).
- Physics, anim, BT were already designed under ADR-0037 to satisfy this contract; Phase 14 simply audits them with the clang-tidy check.

## 4. Alternatives considered

- **Client-authoritative predict-and-reconcile only** — rejected; lockstep is the preferred path for small competitive matches because it never needs a snap-back.
- **GGPO library** — rejected; the library's thread model uses `std::thread`, violating CLAUDE.md #10. We reimplement its shape around `jobs::Scheduler`.
- **Longer rollback window (16 frames)** — rejected; 133 ms is enough for LAN + 100 ms WAN; doubling the window doubles worst-case CPU.
