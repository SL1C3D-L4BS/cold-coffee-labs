# ADR 0030 — Phase 11 cross-cutting decisions (summary)

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 exit gate)
- **Deciders:** Cold Coffee Labs engine group
- **Summarises:** ADRs 0023–0029

This ADR is the **cross-cutting summary** for Phase 11. It pins dependencies, perf gates, and thread-ownership decisions that span more than one wave.

## 1. Dependency pins

| Library | Pin | Gate (CMake option) | Default |
|---|---|---|---|
| RmlUi | 6.2 (2026-01-11) | `GW_UI_RMLUI` | ON for `gw_runtime` / `playable-*`, OFF for `dev-*` |
| HarfBuzz | 14.1.0 (2026-04-04) | `GW_UI_HARFBUZZ` | same as above |
| FreeType | 2.14.3 (2026-03-22) | `GW_UI_FREETYPE` | same as above |
| toml++ | 3.4.0 | `GW_CONFIG_TOMLPP` | same as above |

All four gates default **OFF** on the CI pipelines that do *not* target the Playable Runtime milestone. The stub implementations compile and run identically to the production path for every subsystem test that does not require a real font rasteriser or real TOML round-trip.

**Why gate off by default for `dev-*` CI?** The phase's Definition-of-Done is tested by the stubs + by `playable-*`. The stubs exist so a clean dev-preset checkout never needs the heavy deps; the `playable-*` preset opts every flag in. Same principle as Phase 10 (ADR-0022 §1).

## 2. Hot-path budgets (CI-enforced via `gw_perf_gate`)

| Gate | Target | ADR |
|---|---|---|
| `EventBus::publish` empty | ≤ 12 ns | 0023 |
| `InFrameQueue` drain 1 000 | ≤ 40 µs / 0 B heap | 0023 |
| `CrossSubsystemBus` 1 000 × 3 subs | ≤ 120 µs / 0 B heap | 0023 |
| `CVar<T>::get()` | ≤ 5 ns | 0024 |
| `CVar<T>::set() + publish` | ≤ 200 ns | 0024 |
| `load_domain(~40 entries)` cold | ≤ 2 ms | 0024 |
| `load_domain(~40 entries)` warm | ≤ 0.3 ms | 0024 |
| Console open → first char | ≤ 80 ms | 0025 |
| `help` over 300 items | ≤ 2 ms | 0025 |
| Autocomplete 2-char prefix | ≤ 0.5 ms | 0025 |
| Shape + layout 200-glyph Latin | ≤ 0.2 ms | 0028 |
| Shape + layout 100-glyph CJK | ≤ 0.4 ms | 0028 |
| Glyph atlas delta / frame | ≤ 64 KB steady | 0028 |
| UI pass GPU time | ≤ 0.8 ms | 0027 |
| Runtime boot | ≤ 1.2 s cold / 0.5 s warm | 0029 |

## 3. Thread-ownership

| Thread | Owns | New calls in Phase 11 |
|---|---|---|
| Main | EventBus drain, CVarRegistry writes, UIService::update, ConsoleService::dispatch, RmlUi Context::Update+Render | `rml_render_hal` record, font atlas update |
| Render | `ui_pass` FrameGraphNode draw | `rml_render_hal::RenderCompiledGeometry` |
| Audio | no change | — |
| Jobs | CrossSubsystemBus async dispatch, SDF atlas rasterisation | — |
| I/O | `.rml`, `.rcss`, font reads | — |

**No** `std::thread` / `std::async` introduced. All parallelism flows through `engine/jobs/`.

## 4. What Phase 12 inherits

1. The RmlUi data-model is the wire for physics-anchored HUD tags (health bars over 3D bodies), which Phase 12 lands.
2. `CrossSubsystemBus` is the wire for physics-hit audio cues and physics debug-draw toggles via CVars.
3. The `ui_pass` frame-graph node is the pattern Phase 17's VFX passes copy.

## 5. Cross-platform notes

- **Windows**: RmlUi uses Win32 `OpenClipboard` via `engine/platform/clipboard`; high-DPI `PROCESS_PER_MONITOR_DPI_AWARE_V2` at window creation.
- **Linux / Wayland**: clipboard via `wl_data_device`; DPI from `GDK_SCALE` or Wayland `wl_output::scale`.
- **Steam Deck**: treated as Linux-Wayland; glyph atlas budget relaxed 25% (Deck has 16 GB unified RAM). DualSense rumble continues through SDL3's path (Phase 10).
- **HiDPI**: `ui.dpi.scale` CVar; default auto-detect, round to nearest 0.25.

## 6. What did *not* land in Phase 11 (deferred by design)

| Not landing | Target phase | Why deferred |
|---|---|---|
| ICU runtime + XLIFF | Phase 16 | Heavy dep; LocaleBridge seam reserved |
| HarfBuzz-shaped RTL in RmlUi layout | Phase 16 | HB ready; RmlUi RTL lands at i18n gate |
| Screen-reader hooks (MSAA/IAccessible2/AT-SPI) | Phase 16 | Needs i18n + a11y bundle |
| On-screen virtual keyboard | Phase 16 | Same |
| Tracy panels / ImNodes inspector for event graph | Phase 17/18 | Editor tooling, downstream |
| RmlUi data-model ↔ BLD agent UI edits | Phase 18 | Needs BLD context model |
| Physics-anchored HUD | Phase 12 | Needs Jolt |
| Subtitle rendering with speaker attribution | Phase 16 | Localised content; caption channel seam reserved |
| Voice-chat push-to-talk UI | Phase 14 | GNS wires to `IVoiceCapture`/`IVoicePlayback` first |

## 7. Exit-gate checklist

- [x] Week rows 066–071 all merged behind the `phase-11` tag
- [x] ADRs 0023–0029 land as Accepted
- [x] ADR 0030 lands as the summary (this doc)
- [x] `gw_tests` green with ≥ 50 new event / config / console / UI cases
- [x] `ctest --preset dev-win` / `dev-linux` green
- [x] `ctest --preset playable-windows` / `playable-linux` green when run with Phase-11 deps on
- [x] Performance gates §2 green
- [x] `docs/a11y_phase11_selfcheck.md` signed off
- [x] `docs/05` marks Phase 11 *shipped*
- [x] Git tag `v0.11.0-runtime-2` pushed
