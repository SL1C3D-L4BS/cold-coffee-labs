# ADR 0029 — Playable runtime: `Engine::Impl` boot order, sandbox scene, CI self-test

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11F)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0017…0028, blueprint §Phase-11 exit demo

## Context

Phase 11's gate is binary: a QA tester sits down, plays a 60-second scene, adjusts the text scale, rebinds a key, toggles the dev console, quits, and finds their settings persisted. Every ADR in 0023–0028 must bolt together cleanly in `runtime/main.cpp` for that to happen.

## Decision

### Boot order (RAII-ordered, single-threaded main)

```
0. CLI parse + env (docs/06 §runtime flags)
1. CorePlatform  (engine/platform/window, time, fs, dll)
2. JobSystem     (engine/jobs/)
3. EventBus      (engine/core/events)
4. CVarRegistry  (engine/core/config; loads graphics.toml / audio.toml / input.toml / ui.toml)
5. AudioService  (engine/audio; backend from audio.toml)
6. InputService  (engine/input; profile from input.toml)
7. UIService     (engine/ui; font library + RmlUi context)
8. ConsoleService (engine/console; registers commands against 3–7)
9. SceneLoader + FrameGraph
10. Main loop
```

Destruction is the mirror image. Any constructor failure propagates `gw::core::Result<Engine, BootError>` up to `main`, which logs and exits cleanly — no destruction of partially-constructed subsystems.

### `runtime/main.cpp` CLI

- `--config-dir=<path>` (default: platform save dir; tests override)
- `--self-test --frames=N --golden=<png>`
- `--allow-console=<0|1>`
- `--replay-trace=<path>.input_trace`
- `--record-trace=<path>.input_trace`
- `--scene=<path>.gwscene` (default: `assets/scenes/playable_boot.gwscene`)

### Sandbox playable scene

`apps/sandbox_playable/` ships the 60-second demo: a small 3D room, three audio emitters (ambience loop, crackle one-shot, voice line with a caption-channel publish), a first-person action set, a HUD (hp, interact, caption area), and the dev console bound to backtick/F1.

### CI self-test

`ctest --preset playable-windows` / `playable-linux`:

- Launches `gw_runtime --self-test --frames=120 --golden=tests/golden/playable_120f.png`
- Asserts:
  1. The process exits with code 0 within 30 s
  2. ≥ 3 RmlUi elements drew on frame 120
  3. Audio callback pumped ≥ 120 × 1024 frames (NullBackend deterministic)
  4. Input action `Jump` fired ≥ 1× through the replay trace
  5. ≥ 1 `ConfigCVarChanged` observed on the bus (the self-test toggles `ui.text.scale`)
  6. `graphics.toml` + `audio.toml` + `input.toml` + `ui.toml` are written on exit and round-trip byte-identical to the seed

### Determinism knobs

The self-test seeds:

- `--replay-trace=tests/fixtures/input/playable_demo.input_trace`
- `audio.deterministic = true`
- `input.backend = trace`
- `rng.seed = 0xC0LD_C0FFE3`

and asserts the `.event_trace` output byte-matches `tests/golden/playable_events_120f.event_trace`.

## Perf contract

| Gate | Target | Notes |
|---|---|---|
| Runtime boot (menu visible) | ≤ 1.2 s cold, ≤ 0.5 s warm | on NVMe |
| Frame-pacing on the demo scene at 1080p | 60 FPS nominal | RX 580 |
| Steady-state heap growth | 0 B / frame | traced via jemalloc profile in nightly |

## Tests (≥ 4 in this ADR)

1. Boot-order unit test: each stage constructs with a null-stub predecessor (no RAII leak)
2. CLI parser: unknown flag is an error with line-column diagnostic
3. `--self-test --frames=1` exits 0 with no sound card and no GPU
4. `user.toml`s round-trip through boot → frame-loop → shutdown unchanged

## Alternatives considered

- **One giant `runtime::App` class** — doesn't mirror the blueprint's `Engine::Impl` shape; harder to test subsystems in isolation.
- **Run the self-test as a plain unit test** — cannot exercise the full boot path + RmlUi + audio callback + input trace in one process; end-to-end exit-code assertion is the minimum useful signal.

## Consequences

- `docs/BUILDING.md` gains a `Playable Runtime` section documenting the new presets.
- The asset pipeline gains the `assets/scenes/playable_boot.gwscene` fixture; its cook outputs pin the 120-frame golden.
