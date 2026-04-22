# Phase 16 — perf budgets

Enforced by `tests/perf/phase16_perf_gate.cpp` (CTest label `phase16;perf`).

| Metric | Target (`dev-*`) | Target (`ship-*`) | Notes |
|---|---|---|---|
| `PlatformServicesWorld::step()` main-thread cost | ≤ 100 µs | ≤ 500 µs | Dev-local has no SDK work |
| `IPlatformServices::publish_event` rate-limit check | ≤ 1 µs | ≤ 5 µs | Ring-buffer lookup |
| `.gwstr` load (10 k keys) | ≤ 300 µs | ≤ 1 ms | Single mmap-style pass |
| `LocaleBridge::resolve` hit | ≤ 200 ns | ≤ 400 ns | Binary search, contiguous table |
| `bidi_segments` (256 scalars, mixed) | ≤ 2 µs | ≤ 8 µs | Null is 1-run pass-through; ICU is UBA-15 |
| `format_message` (3 args, plural) | ≤ 5 µs | ≤ 20 µs | Null subset supports `{k}`, `{k, plural, ...}`, `{k, number}` |
| `TextShaper::shape` (Latin, 80 scalars) | ≤ 30 µs | ≤ 50 µs | Null 1:1; HarfBuzz real shape |
| `a11y::selfcheck()` | ≤ 50 µs | ≤ 200 µs | Walks live CVar list |
| `a11y::SubtitleQueue::push` | ≤ 500 ns | ≤ 1 µs | Sorted-insert, cap 32 cues |
| `a11y::IScreenReader::update_tree` (50 nodes) | ≤ 5 µs | ≤ 20 µs | Null stores delta; AccessKit writes IPC batch |

The `gw_perf_gate_phase16` binary loops each operation `N=1024` times,
takes the median, and fails if any target is breached. Debug builds
apply a 4× slack to stay green on sanitizer presets.
