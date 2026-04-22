# Phase 15 performance budgets

CI enforcement target: label `phase15` (see ADR-0064). Measurements use `gw_perf_gate` / wall-clock micro-benchmarks on RX-580-class dev PCs.

| Scenario | Budget |
|---|---|
| `.gwsave` write, 2 048 entities, 9 chunks, zstd-3 | ≤ 18 ms main + ≤ 22 ms IO lane |
| `.gwsave` header peek | ≤ 600 µs |
| `.gwsave` full load, 2 048 entities | ≤ 14 ms main + 14 ms IO lane |
| ECS snapshot serialize, 2 048 entities | ≤ 6 ms |
| ECS snapshot deserialize | ≤ 8 ms |
| Migration v1→v2 in-place | ≤ 3 ms |
| BLAKE3-256 footer (2 MiB payload) | ≤ 2.5 ms |
| zstd compress (2 MiB, level 3) | ≤ 6 ms |
| SQLite slot insert | ≤ 0.4 ms |
| SQLite telemetry_queue insert | ≤ 0.3 ms |
| Telemetry flush (64 × 1 KiB events) | ≤ 4 ms main + ≤ 40 ms POST |
| Sentry crash handler cold init | ≤ 35 ms |
| Autosave main-thread spike | ≤ 2 ms |
| `tele.dsar.export` (50 slots) | ≤ 400 ms |
| Telemetry compiled out overhead | ≤ 60 µs |
