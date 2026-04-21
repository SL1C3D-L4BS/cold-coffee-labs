# ADR 0055 — Phase 14 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 14 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0047–0054

This ADR is the cross-cutting summary for Phase 14 — dependency pins, perf gates, thread ownership, CI layout, replay format, a11y, and hand-off to Phase 15.

## 1. Dependency pins

| Library | Pin | Gate | Default |
|---|---|---|---|
| GameNetworkingSockets | v1.4.1 (2024-03-25) | `GW_ENABLE_GNS` → `GW_NET_GNS` | OFF for `dev-*`, ON for `net-*` |
| Opus | v1.5.2 (2024-05-15) | `GW_ENABLE_OPUS` → `GW_NET_OPUS` | OFF / ON |
| Steam Audio | reuses Phase 10 `GW_ENABLE_PHONON` | inherits | OFF / ON |

GNS transitively pulls OpenSSL + Protobuf; these are header-quarantined inside `engine/net/impl/` and never re-exported through public headers.

Clean-checkout `dev-*` builds the null transport + null Opus passthrough + constant-power panning (no Phonon); every Phase-14 code path is still reachable and every unit test is deterministic.

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Scenario | Budget | ADR |
|---|---|---|
| Replication step (2c+server, 2 048 entities, 60 Hz) | ≤ 1.6 ms server main thread | 0048 |
| Snapshot serialize (1 snap, 512 deltas) | ≤ 400 µs | 0048 |
| Snapshot deserialize + apply | ≤ 450 µs | 0048 |
| Delta bitmap scan (4 096 fields) | ≤ 120 µs | 0048 |
| Priority DRR schedule (2 048 comps, 32 KiB budget) | ≤ 80 µs | 0049 |
| Lag-comp archive (per frame) | ≤ 1.1 ms | 0050 |
| Lag-comp rewind (12-frame scope) | ≤ 2.2 ms | 0050 |
| Opus encode 20 ms CBR 24 kbps c5 | ≤ 1.1 ms | 0052 |
| Opus decode 20 ms | ≤ 0.4 ms | 0052 |
| Voice spatializer (HRTF, 48 kHz mono → stereo) | ≤ 1.0 ms | 0052 |
| `determinism_hash` (1 024 bodies) | ≤ 80 µs | ADR-0037 reused |
| Bandwidth down per peer @ 2c+server | ≤ 32 KiB/s | 0048 |
| Bandwidth down per peer @ 16c | ≤ 96 KiB/s | 0048 |
| Voice round-trip LAN | ≤ 150 ms | 0052 |
| LockstepSession SyncTest (1 rollback, 60 ticks) | ≤ 4 ms | 0054 |

All gates ship under CTest label `phase14`. Any GNS / Opus pin bump reruns every gate.

## 3. Thread-ownership summary

| Thread | Owns |
|---|---|
| Main | `NetworkWorld` lifetime + public API; drains `InFrameQueue<Net*>` buses |
| Jobs — `sim` lane | Replicate job, delta encode/decode, voice encode/decode batches |
| Jobs — `net_io` lane (linked with GNS) | GNS poll + packet demux |
| Jobs — `background` lane | Session dev-local file scan, `.gwreplay` flush |

Zero `std::thread` / `std::async`. The `net_io` lane is a named lane inside `jobs::Scheduler`, not a private thread.

## 4. Tick ordering (enforced in `runtime::Engine::tick`)

```
physics.step()
 → anim.step()
 → bt.tick()
 → nav.drain_bake_results()
 → net.step()                      // replicate_job, voice encode, linksim deliver
 → ecs.transform_sync_system()
 → render.extract()
```

Lag compensation's `scoped_rewind` is the only place where physics state briefly diverges from this pipeline; it always restores within the same tick.

## 5. `.gwreplay` v3 — `NETW` section

Extends `.gwreplay` v2 (ADR-0037) with a per-peer input + ack stream, making a full multiplayer session deterministically replayable.

```
v3 header adds:
    netw_version : u16    // = 3
    peer_count   : u32
per peer:
    peer_id      : u32
    seed         : u64
    inputs       : framed [ (tick u32, blob_len u16, blob bytes) ... ]
    acks         : framed [ (tick u32, baseline_id u32) ... ]
    hash_stream  : [ (tick u32, hash u64) ... ]
```

The server records both its local input stream (ADR-0037) and each connected peer's inputs + ack stream. `ReplayPlayer` re-drives the server against the null transport, replaying each peer's input at the recorded tick. Success criterion: per-frame `determinism_hash` identical to the recorded.

## 6. CI pipeline (additions)

```
.github/workflows/ci.yml:
  phase14-windows:
    needs: phase13-windows
    - cmake --preset dev-win && ctest --preset dev-win -L "phase14|smoke"
  phase14-linux:
    needs: phase13-linux
    - cmake --preset dev-linux && ctest --preset dev-linux -L "phase14|smoke"
  phase14-net-smoke-windows:
    needs: phase14-windows
    if: gns_enabled
    - cmake --preset net-win && ctest --preset net-win -L gns
  phase14-net-smoke-linux:
    needs: phase14-linux
    if: gns_enabled
    - cmake --preset net-linux && ctest --preset net-linux -L gns
  phase14-replay-cross-platform:
    needs: [phase14-windows, phase14-linux]
    - download .gwreplay artefact from Windows job
    - ./gw_tests --test-case="net replay cross-platform parity"
  phase14-two-client-night:
    needs: phase14-windows
    - ./sandbox_netplay --role=linkroom --rtt_ms=120 --loss_pct=2 --ticks=10000
    - grep -q "NETPLAY OK" output
    - grep -q "desyncs=0" output
```

`phase14-replay-cross-platform` and `phase14-two-client-night` are required before merge on any PR touching `engine/net/`, `engine/physics/`, or `engine/anim/`.

## 7. Accessibility self-check (`docs/a11y_phase14_selfcheck.md`)

- Voice chat defaults to opt-in push-to-talk; voice-activity mode is opt-in; mute-all-remote is one-click; per-peer mute list persists via `engine/core/config`.
- STT caption hook (`IVoiceTranscriber`, no-op default) — Phase-16 ships a real backend that feeds the subtitle system.
- Color-blind-safe debug palettes for `net.debug` overlays share the 8-entry palette used by `physics.debug` and `nav.debug`.
- Keyboard-first authoring — join-by-session-code UI is fully tab-reachable.
- Packet-loss icon pulse is capped at 2 Hz and respects `ui.reduce_motion`.

## 8. Determinism ladder

1. ADR-0037 `determinism_hash` is the oracle: every peer emits one per tick, every tick is compared.
2. The replication bit-packer is fixed-width and deterministic — no allocator-dependent ordering.
3. LinkSim uses a seeded RNG keyed on `LinkProfile::seed` + `tick` — same seed + same send sequence = same drops.
4. Lockstep SyncTest (`run_sync_test`) is CI-mandatory; any rollback-pure violation fails the gate.
5. `.gwreplay` v3 artifacts from Windows replay byte-identically on Linux.

## 9. Upgrade-gate procedure (GNS / Opus / Steam Audio)

Any PR that bumps a Phase-14 dep pin **must**:

1. Re-run `phase14-replay-cross-platform` against the existing golden `.gwreplay`s — bytes must match.
2. Re-run `phase14-two-client-night` at `{120 ms, 2 %}` — zero desyncs.
3. Re-run `phase14-net-smoke-*` on both OSes.
4. Annotate the new pin in `docs/adr/0055-phase14-cross-cutting.md` §1 with the upgrade date.

## 10. Risks & retrospective

| Risk | Mitigation | Phase-14 outcome |
|---|---|---|
| GNS upstream changes break header quarantine | pin v1.4.1; wrap behind `ITransport`; `impl/` gate | null-backend CI green on clean checkout |
| Cross-platform FP non-determinism in quantizers | bit-quantize everything; never send `f64` | `phase14-replay-cross-platform` passes on null |
| Opus license clash | BSD-3, CI-safe | confirmed matches Jolt / ACL / Recast license surface |
| Steam Audio HRTF cost on RX 580 | Opus complexity 5; voice bus can fall back to pan | fallback exercised by `voice_spatial_fallback_test` |
| NAT traversal failure | GNS STUN first, session backend relay (Phase 16) | dev-local never needs NAT |
| Desync inside rollback window | SyncTest CI-mandatory; `GW_ROLLBACK_PURE` clang-tidy rule | `lockstep_two_client_determinism_test` green |
| Voice + replication bandwidth collision | per-peer DRR scheduler (ADR-0049); voice priority below combat | `priority_starvation_test` green |

## 11. Hand-off to Phase 15 (Persistence & Telemetry)

Phase 15 inherits two locked contracts:

- **`.gwreplay` v3** — Phase 15 telemetry events extend (not break) the v3 stream; a new `TELE` section is appended.
- **Per-peer session seed** (ADR-0053 §2.3) — Phase 15 save file includes the session seed; reloading a save into a new session rebinds the RNGs consistently.

The `net-*` preset is the base for `persist-*` in Phase 15.

## 12. Exit-gate checklist

- [x] ADRs 0047–0055 landed with Status: Accepted.
- [x] `engine/net/` module tree matches ADR §2 / §4.
- [x] Headers contain zero `<steam/**>`, `<opus.h>`, `<phonon.h>` in public API.
- [x] `determinism_hash` comparator wired through `DesyncDetector` + CI.
- [x] 26 Phase-14 CVars registered; 10 console commands registered.
- [x] `sandbox_netplay` prints `NETPLAY OK`; CTest `two_client_night_ctest` green.
- [x] ≥ 30 new Phase-14 tests; total CTest count ≥ 377 on `dev-win`.
- [x] Roadmap Phase 14 → `completed` with closeout paragraph.
- [x] Risk-table retrospective filled in §10.
