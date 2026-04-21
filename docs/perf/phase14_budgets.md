# docs/perf/phase14_budgets.md — Phase 14 performance gates

**Status:** Operational (CI-enforced)
**Owner:** Networking Lead
**Enforced by:** `gw_perf_gate` target + per-unit-test budget assertions

Every Phase-14 PR runs these gates. A red gate blocks merge.

## Reference hardware

- CI box: Windows 11 / Ubuntu 24.04, clang-cl 22.1.3 / clang 18, RX 580 class, on-host loopback.
- Measurement: steady-state average across 120 samples after 60-sample warmup.
- Wall clock taken via `std::chrono::steady_clock` for host-side measurements; simulation tick math runs on the fixed-step clock, never wall-clock (ADR-0037 §2 / ADR-0054 §2.4).
- Bandwidth budgets measured as bytes across `ITransport::send` over a rolling 1 s window.

## Budgets

| Id | Scenario | Budget | ADR | CTest label |
|---|---|---|---|---|
| P14.R1 | Replication step — 2c+server, 2 048 entities, 60 Hz | ≤ 1.60 ms (server main thread) | 0048 | phase14;net |
| P14.R2 | Snapshot serialize — 1 snap, 512 deltas | ≤ 400 µs | 0048 | phase14;net |
| P14.R3 | Snapshot deserialize + apply | ≤ 450 µs | 0048 | phase14;net |
| P14.R4 | Delta bitmap scan — reflection-driven, 4 096 fields | ≤ 120 µs | 0048 | phase14;net |
| P14.R5 | Priority DRR schedule — 2 048 comps @ 32 KiB budget | ≤ 80 µs | 0049 | phase14;net |
| P14.L1 | Lag-comp archive — per server frame | ≤ 1.10 ms | 0050 | phase14;net |
| P14.L2 | Lag-comp rewind — 12-frame scope + raycast | ≤ 2.20 ms | 0050 | phase14;net |
| P14.L3 | Lag-comp hash restore roundtrip | ≤ 80 µs | 0050 | phase14;determinism |
| P14.V1 | Opus encode — 20 ms CBR 24 kbps complexity 5 | ≤ 1.10 ms | 0052 | phase14;voice |
| P14.V2 | Opus decode — 20 ms | ≤ 0.40 ms | 0052 | phase14;voice |
| P14.V3 | Voice HRTF spatialize — 48 kHz mono → stereo | ≤ 1.00 ms | 0052 | phase14;voice |
| P14.V4 | Voice round-trip LAN, null transport | ≤ 150 ms | 0052 | phase14;voice |
| P14.H1 | `determinism_hash` fold — 1 024 bodies | ≤ 80 µs | 0037 (reused) | phase14;determinism |
| P14.H2 | `DesyncDetector::push_hash` | ≤ 3 µs | 0051 | phase14;determinism |
| P14.B1 | Bandwidth down per peer @ 2c+server, steady state | ≤ 32 KiB/s | 0048 | phase14;net |
| P14.B2 | Bandwidth down per peer @ 16c, steady state | ≤ 96 KiB/s | 0048 | phase14;net |
| P14.S1 | LockstepSession SyncTest — 60 ticks, 1 rollback | ≤ 4.00 ms | 0054 | phase14;determinism |
| P14.S2 | LockstepSession SyncTest — 600 ticks, 8-frame window | ≤ 20.0 ms | 0054 | phase14;determinism |
| P14.T1 | `ITransport` send+poll roundtrip, null loopback | ≤ 0.50 µs / packet | 0047 | phase14;net |
| P14.T2 | LinkSim schedule — 1 000 in-flight packets | ≤ 200 µs / drain | 0051 | phase14;net |

## Null-backend guarantees

All budgets above are measured on the **null transport + null Opus + constant-power panning** stack — the gates that must green on every `dev-*` CI run. With `GW_NET_GNS` on the wire-I/O jobs become real poll loops; the budget for those jobs is documented in `docs/perf/phase14_budgets.md §Net-I/O` once Phase 14's GNS bring-up smoke is green on a real LAN.

Null Opus is a memcpy passthrough; it completes P14.V1/V2 in ≤ 2 µs per frame. The Opus gates are evaluated when `GW_NET_OPUS` is linked.

## Bandwidth accounting

The DRR scheduler (ADR-0049) debits bytes against `net.send_budget_kbps` on every send. P14.B1 / P14.B2 are reported as the steady-state sum of bytes sent per peer, averaged across a 1 s rolling window. A violation fails `gw_perf_gate` with the offending per-component histogram attached.

## Upgrade procedure

Any pin bump (GNS / Opus / Steam Audio) reruns **all 19 gates** as part of the PR. Table updates land in this file + ADR-0055 §1.
