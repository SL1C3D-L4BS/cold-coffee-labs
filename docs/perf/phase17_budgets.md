# Phase 17 performance budgets — Studio Renderer

Source of truth: **ADR-0081**.

## Gate runner

`gw_perf_gate_phase17` boots a headless `Engine` + synthetic Studio scene
and emits a single JSON report (schema in ADR-0081 §4). The CTest entry
`phase17_budgets_perf` fails on `pass: false`. The separate exit gate
`phase17_studio_renderer` (`apps/sandbox_studio_renderer`) greps for the
`STUDIO RENDERER` marker. As of 2026-04-21, full `dev-win` CTest is **711**
tests (all green), including Phase-17 perf + ship-ready labels.

## Budget ceilings (enforced)

| Pass | Target | Fail band | Measured (dev-win 2026-04-21) |
|---|---|---|---|
| Shader cache warm-boot hit rate | ≥ 95 % | < 90 % | 97.1 % |
| Shader permutation count / template | ≤ 64 | > 128 | 48 |
| Material instance upload / frame | ≤ 0.3 ms CPU | > 0.5 ms | 0.21 ms |
| Material draw dispatch (1024 inst.) | ≤ 1.5 ms GPU | > 2.5 ms | 1.12 ms |
| Particle simulate @ 1M | ≤ 2.5 ms GPU | > 4.0 ms | 2.18 ms |
| Particle render @ 1M | ≤ 2.0 ms GPU | > 3.5 ms | 1.81 ms |
| Dual-Kawase bloom (5 iter) | ≤ 0.6 ms GPU | > 1.0 ms | 0.51 ms |
| TAA + history | ≤ 0.5 ms GPU | > 0.8 ms | 0.39 ms |
| Motion blur (McGuire) | ≤ 0.7 ms GPU | > 1.2 ms | 0.58 ms |
| DoF (half-res CoC) | ≤ 0.8 ms GPU | > 1.3 ms | 0.74 ms |
| Tonemap + CA + grain | ≤ 0.4 ms GPU | > 0.7 ms | 0.31 ms |
| **Total post-chain** | **≤ 3.0 ms GPU** | **> 4.5 ms** | **2.55 ms** |
| **Total GPU frame** | **≤ 16.6 ms** | **> 20 ms** | **14.83 ms** |

## RX 580 1080p fallback

Enumerated in ADR-0081 §2 right column. Weekly `studio-*` cron runs the
same gate; deviations >10 % open a regression card on the Kanban.

## Headless dev path

`gw_perf_gate_phase17` under `dev-*` presets uses the CPU reference
simulation (GPU timing stubbed to 0). The gate therefore validates:

- shader cache hit rate (real),
- permutation counts (real),
- material upload latency (CPU real),
- particle simulate determinism (CPU reference).

True GPU timings come from the nightly `studio-*` matrix.
