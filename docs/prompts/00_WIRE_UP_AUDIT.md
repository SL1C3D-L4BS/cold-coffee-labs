# Wire-Up Audit — Integration Gaps

Status: `2026-04-22`  |  Authoritative list feeding `docs/prompts/preflight/` tasks.

This report catalogues every scaffolded-but-not-called module in the
repository. Each entry names the orphan symbol(s), the expected call-site,
the minimum wire-up, and any blockers. The prompt pack (`pack.yml`) turns
each entry into a concrete Phase 20.5 preflight task.

**Totals (2026-04-22):** 24 integration gaps across 4 subsystems.

| Subsystem            | Gaps | Preflight prefix     |
|----------------------|------|----------------------|
| Editor boot          | 7    | `pre-ed-*`           |
| Engine runtime       | 6    | `pre-eng-*`          |
| Gameplay             | 5    | `pre-gp-*`           |
| Tools & CI           | 6    | `pre-tc-*`           |

---

## 1. Editor boot (7 gaps)

### `pre-ed-theme-menu` — Theme registry + `View → Theme` menu

- **Orphan**: [editor/theme/theme_registry.hpp](../../editor/theme/theme_registry.hpp),
  [editor/theme/theme_registry.cpp](../../editor/theme/theme_registry.cpp).
  `gw::editor::theme::ThemeRegistry::instance()` exists but no code calls
  `set_active()` at startup and no menu path invokes it at runtime.
- **Expected call-site**: [editor/app/editor_app.cpp](../../editor/app/editor_app.cpp)
  `apply_theme()` at line 1334 and the main-menu builder inside `build_ui()`.
- **Minimum wire-up**:
  1. Read persisted `ThemeId` from editor TOML config on startup and call
     `ThemeRegistry::instance().set_active(id)` before `apply_theme()`.
  2. Add a `View → Theme` submenu that enumerates `BrewedSlate`,
     `CorruptedRelic`, `FieldTestHC` and calls `set_active()` on selection.
  3. Re-run `apply_theme()` when the active id changes.
- **Blocked by**: none.

### `pre-ed-sacrilege-panels` — Sacrilege panel registration

- **Orphan**: 10 panels under [editor/panels/sacrilege/](../../editor/panels/sacrilege/)
  (act_state_panel, ai_director_sandbox_panel, circle_editor_panel,
  dialogue_graph_panel, editor_copilot_panel, encounter_editor_panel,
  material_forge_panel, pcg_node_graph_panel, shader_forge_panel,
  sin_signature_panel). All headers + stubs compile; none are registered.
- **Expected call-site**: [editor/app/editor_app.cpp](../../editor/app/editor_app.cpp)
  `init_imgui()` → the `panels_.add(std::make_unique<…>)` block around
  lines 219-233.
- **Minimum wire-up**: one `panels_.add(std::make_unique<SacrilegePanelX>())`
  per panel. Panels that need the world, render settings, or selection take
  pointers to `scene_world_`, `render_settings_`, `selection_`.
- **Blocked by**: none (panels construct default).

### `pre-ed-audit-panels` — Audit panel registration

- **Orphan**: 7 panels under [editor/panels/audit/](../../editor/panels/audit/)
  (shader_hotreload_panel, profiler_panel, asset_deps_panel,
  localization_panel, heatmap_panel, bake_panel,
  shader_permutations_panel).
- **Expected call-site**: same block in `editor/app/editor_app.cpp` around
  line 226.
- **Minimum wire-up**: same shape as `pre-ed-sacrilege-panels`.
- **Blocked by**: none.

### `pre-ed-module-manifest` — `editor/modules/` manifest boot

- **Orphan**: [editor/modules/module_manifest.hpp](../../editor/modules/module_manifest.hpp),
  [editor/modules/modules_builtin.cpp](../../editor/modules/modules_builtin.cpp).
  `gw::editor::modules::register_builtin_modules()` exists but is never
  called by the editor startup.
- **Expected call-site**: [editor/app/editor_app.cpp](../../editor/app/editor_app.cpp)
  `EditorApplication::EditorApplication()` or `init_imgui()` before the
  `panels_.add(...)` block.
- **Minimum wire-up**: `gw::editor::modules::register_builtin_modules();`
  then drive `panels_.add(...)` from the manifest rather than hand-rolled
  code (follow-up refactor — initial fix just wires the call).
- **Blocked by**: `pre-ed-sacrilege-panels`, `pre-ed-audit-panels`.

### `pre-ed-a11y-init` — `editor/a11y/` init + Window-menu entry

- **Orphan**: [editor/a11y/editor_a11y.hpp](../../editor/a11y/editor_a11y.hpp),
  [editor/a11y/editor_a11y.cpp](../../editor/a11y/editor_a11y.cpp).
  `EditorA11yConfig` + `apply()` defined; never called.
- **Expected call-site**: `editor/app/editor_app.cpp` `init_imgui()` after
  `apply_theme()`.
- **Minimum wire-up**:
  1. Construct `EditorA11yConfig`, load from TOML, call `apply(cfg)`.
  2. Add `Window → Accessibility` menu entry opening a modal toggle panel.
- **Blocked by**: none.

### `pre-ed-pie-overlays` — PIE debug HUD / perf guard / rollback inspector

- **Orphan**: [editor/pie/pie_debug_hud.hpp](../../editor/pie/pie_debug_hud.hpp),
  [editor/pie/pie_perf_guard.hpp](../../editor/pie/pie_perf_guard.hpp),
  [editor/pie/rollback_inspector.hpp](../../editor/pie/rollback_inspector.hpp).
  Neither is called by `editor/pie/gameplay_host.cpp`.
- **Expected call-site**: [editor/pie/gameplay_host.cpp](../../editor/pie/gameplay_host.cpp)
  inside `tick()` (HUD + perf guard) and `stop()` (rollback inspector).
- **Minimum wire-up**:
  1. Own one instance of each overlay in `GameplayHost`.
  2. `PieDebugHud::render()` from `tick()` when `in_play()`.
  3. `PiePerfGuard::sample()` each tick; warn when over-budget.
  4. `RollbackInspector::capture()` on PIE stop.
- **Blocked by**: none.

### `pre-ed-widgets-firstuse` — First use of `editor/widgets/*`

- **Orphan**: [editor/widgets/distressed_widgets.hpp](../../editor/widgets/distressed_widgets.hpp),
  [editor/widgets/spline_editor.hpp](../../editor/widgets/spline_editor.hpp).
- **Expected call-site**: the Circle Editor panel (lands with
  `pre-ed-sacrilege-panels`) — natural host for distressed widgets;
  spline_editor lands inside the Encounter Editor panel's spawn-path UI.
- **Minimum wire-up**: one `distressed_button(...)` call in
  `circle_editor_panel.cpp` and one spline editing surface in
  `encounter_editor_panel.cpp`.
- **Blocked by**: `pre-ed-sacrilege-panels`.

---

## 2. Engine runtime (6 gaps)

### `pre-eng-ai-runtime-dispatch` — AI runtime dispatched from services

- **Orphan**: [engine/ai_runtime/director_policy.hpp](../../engine/ai_runtime/director_policy.hpp),
  [engine/ai_runtime/music_symbolic.hpp](../../engine/ai_runtime/music_symbolic.hpp),
  [engine/ai_runtime/material_eval.hpp](../../engine/ai_runtime/material_eval.hpp).
- **Expected call-site**:
  - [engine/services/director/director_service.cpp](../../engine/services/director/director_service.cpp)
    → `director_policy::tick()`
  - [engine/services/audio_weave/audio_weave.cpp](../../engine/services/audio_weave/audio_weave.cpp)
    → `music_symbolic::step()`
  - [engine/services/material_forge/material_forge.cpp](../../engine/services/material_forge/material_forge.cpp)
    → `material_eval::evaluate()`
- **Minimum wire-up**: three `#include` + one call site each.
- **Blocked by**: none.

### `pre-eng-narrative-tick` — narrative tick in gameplay world

- **Orphan**: [engine/narrative/act_state.hpp](../../engine/narrative/act_state.hpp),
  [engine/narrative/dialogue_graph.hpp](../../engine/narrative/dialogue_graph.hpp),
  [engine/narrative/sin_signature.hpp](../../engine/narrative/sin_signature.hpp),
  [engine/narrative/voice_director.hpp](../../engine/narrative/voice_director.hpp),
  [engine/narrative/grace_meter.hpp](../../engine/narrative/grace_meter.hpp).
- **Expected call-site**: gameplay world tick (the common ancestor of every
  sandbox main — we register a `NarrativeTickSystem` that owns all five).
- **Minimum wire-up**:
  1. New `engine/narrative/narrative_tick_system.hpp/.cpp` that advances
     act state, dialogue graph, sin signature, voice director, grace meter
     in that order.
  2. Gameplay sandbox registers the system during world init.
- **Blocked by**: none.

### `pre-eng-franchise-services` — 6 franchise services reached from owners

- **Orphan**: [engine/services/material_forge](../../engine/services/material_forge/material_forge.hpp),
  [level_architect](../../engine/services/level_architect/level_architect.hpp),
  [combat_simulator](../../engine/services/combat_simulator/combat_simulator.hpp),
  [gore](../../engine/services/gore/gore_service.hpp),
  [audio_weave](../../engine/services/audio_weave/audio_weave.hpp),
  [director](../../engine/services/director/director_service.hpp).
  (Editor Co-Pilot covered by `pre-ed-sacrilege-panels`.)
- **Expected call-site**: each service is reached by its owning subsystem
  on world init: material forge from render init, level architect from
  world init, combat simulator from combat system init, gore from damage
  system init, audio weave from audio init, director from gameplay loop.
- **Minimum wire-up**: one `ServiceX::instance()` call per owner.
- **Blocked by**: `pre-eng-ai-runtime-dispatch` (director/audio_weave
  branches need AI runtime wired first).

### `pre-eng-crash-reporter-init` — `engine/core/crash_reporter` at `main()`

- **Orphan**: [engine/core/crash_reporter.hpp](../../engine/core/crash_reporter.hpp),
  [engine/core/crash_reporter.cpp](../../engine/core/crash_reporter.cpp).
- **Expected call-site**: every app `main()` — editor, sandboxes, cook
  worker.
- **Minimum wire-up**: `gw::core::crash_reporter::install()` as first line
  of `main()`.
- **Blocked by**: none.

### `pre-eng-steam-input-glue` — Steam Input plugged into input service

- **Orphan**: [engine/input/steam_input_glue.hpp](../../engine/input/steam_input_glue.hpp),
  [engine/input/steam_input_glue.cpp](../../engine/input/steam_input_glue.cpp).
- **Expected call-site**: [engine/input/input_system.hpp](../../engine/input/input_system.hpp)
  (or equivalent dispatcher) during `init()` when Steam is available.
- **Minimum wire-up**: query `steam_input_glue::available()`; if true, own
  a `SteamInputSource` and poll it each frame alongside GLFW keys.
- **Blocked by**: none.

### `pre-eng-fsr2-hal-frame-graph` — FSR2 + HAL inserted into frame graph

- **Orphan**: [engine/render/post/fsr2/](../../engine/render/post/fsr2/) stubs,
  [engine/render/hal/hdr_output.hpp](../../engine/render/hal/hdr_output.hpp),
  [engine/render/hal/present_timing.hpp](../../engine/render/hal/present_timing.hpp).
- **Expected call-site**: [engine/render/frame_graph/frame_graph.cpp](../../engine/render/frame_graph/frame_graph.cpp)
  `build()` — FSR2 upscale pass between scene-color and tonemap;
  `hdr_output` + `present_timing` in swapchain acquire/present.
- **Minimum wire-up**: register two passes in the frame-graph builder and
  gate them on a new `RenderSettings::fsr2_enabled` / `hdr_enabled` flag.
- **Blocked by**: none.

---

## 3. Gameplay (5 gaps)

### `pre-gp-martyrdom-ecs` — Martyrdom / God Mode / Blasphemy ECS registration

- **Orphan**: [gameplay/martyrdom/god_mode.hpp](../../gameplay/martyrdom/god_mode.hpp),
  [gameplay/martyrdom/blasphemy_state.hpp](../../gameplay/martyrdom/blasphemy_state.hpp),
  [gameplay/martyrdom/grace_meter.hpp](../../gameplay/martyrdom/grace_meter.hpp).
- **Expected call-site**: gameplay world `register_components()`.
- **Minimum wire-up**: `world.register_component<SinComponent>()` and
  friends; landed for real in P22 W143.
- **Blocked by**: `p22-w143-ecs-components`.

### `pre-gp-acts-tick` — Acts state-machine tick

- **Orphan**: [gameplay/acts/act_state_component.hpp](../../gameplay/acts/act_state_component.hpp),
  [gameplay/acts/i/act_i_rite_of_unspeaking.hpp](../../gameplay/acts/i/act_i_rite_of_unspeaking.hpp),
  [gameplay/acts/ii/act_ii_blasphemy_circuit.hpp](../../gameplay/acts/ii/act_ii_blasphemy_circuit.hpp),
  [gameplay/acts/iii/act_iii_unmaking.hpp](../../gameplay/acts/iii/act_iii_unmaking.hpp).
- **Expected call-site**: gameplay world tick (order: after Martyrdom
  accrual, before voice director).
- **Minimum wire-up**: new `ActsStateSystem` that transitions act via
  thresholds (done as a back-link inside P22 W145).
- **Blocked by**: `p22-w145-blasphemy`.

### `pre-gp-logos-phase4` — Logos phase-4 selector hook

- **Orphan**: [gameplay/boss/logos/phase4_selector.hpp](../../gameplay/boss/logos/phase4_selector.hpp).
- **Expected call-site**: Phase 23 boss arena (Logos); wire-stub now
  lands as a `weak_ptr` owner inside `engine/services/director/`.
- **Minimum wire-up**: director owns an optional `Logos::Phase4Selector`
  that's constructed when the Logos arena loads.
- **Blocked by**: none (stub-wire only — real logic is Phase 23).

### `pre-gp-malakor-owner` — Malakor / Niccolò character state owner

- **Orphan**: [gameplay/characters/malakor_niccolo/character_state.hpp](../../gameplay/characters/malakor_niccolo/character_state.hpp).
- **Expected call-site**: `engine/narrative/voice_director.cpp` owns one
  instance and feeds it per-tick (mirror-step ability at Sin ≥ 80).
- **Minimum wire-up**: promote `CharacterState` into a member of
  `VoiceDirector`; landed for real in P22 W146.
- **Blocked by**: `p22-w146-player-controller`.

### `pre-gp-a11y-init` — `gameplay/a11y` init + HUD surface

- **Orphan**: [gameplay/a11y/gameplay_a11y.hpp](../../gameplay/a11y/gameplay_a11y.hpp).
- **Expected call-site**: gameplay world init plus the HUD surface.
- **Minimum wire-up**: `gw::gameplay::a11y::initialize_defaults(cfg)` +
  `apply(cfg)` at init; HUD reads `cfg` to gate gore level and colour-blind
  overrides; landed for real in P21 W140.
- **Blocked by**: `p21-w140-hud-editor-panels`.

---

## 4. Tools & CI (6 gaps)

### `pre-tc-content-signing` — `tools/cook/content_signing` called by cook worker

- **Orphan**: [tools/cook/content_signing.hpp](../../tools/cook/content_signing.hpp),
  [tools/cook/content_signing.cpp](../../tools/cook/content_signing.cpp).
- **Expected call-site**: [tools/cook/cook_worker.cpp](../../tools/cook/cook_worker.cpp)
  inside the post-cook finalisation step.
- **Minimum wire-up**: on successful cook, call
  `content_signing::sign_artifact(path, private_key)`; record signature in
  cook manifest.
- **Blocked by**: none.

### `pre-tc-fuzz-preset` — `GW_FUZZ=ON` preset in `CMakePresets.json`

- **Orphan**: [tests/fuzz/CMakeLists.txt](../../tests/fuzz/CMakeLists.txt)
  (harnesses exist), fuzz workflow
  [.github/workflows/fuzz.yml](../../.github/workflows/fuzz.yml) (expects
  a preset that does not exist in `CMakePresets.json`).
- **Expected call-site**: `CMakePresets.json` top-level `configurePresets`.
- **Minimum wire-up**: add a `dev-fuzz-clang` preset with
  `"GW_FUZZ": "ON"`, clang toolchain, `AddressSanitizer;UndefinedBehaviorSanitizer`
  enabled.
- **Blocked by**: none.

### `pre-tc-tests-split` — `tests/split/*.cmake` integrated into `tests/CMakeLists.txt`

- **Orphan**: [tests/split/gw_tests_ai.cmake](../../tests/split/gw_tests_ai.cmake),
  [gw_tests_physics.cmake](../../tests/split/gw_tests_physics.cmake),
  [gw_tests_net.cmake](../../tests/split/gw_tests_net.cmake).
- **Expected call-site**: [tests/CMakeLists.txt](../../tests/CMakeLists.txt).
- **Minimum wire-up**: `if(GW_TESTS_SPLIT) include(tests/split/gw_tests_ai.cmake) …`
  behind a new `GW_TESTS_SPLIT=ON` option.
- **Blocked by**: none.

### `pre-tc-bld-tool-dispatch` — `bld-tools/src/sacrilege_tools.rs` in BLD dispatcher

- **Orphan**: [bld/bld-tools/src/sacrilege_tools.rs](../../bld/bld-tools/src/sacrilege_tools.rs).
- **Expected call-site**: [bld/bld-tools/src/lib.rs](../../bld/bld-tools/src/lib.rs)
  `tool_dispatch()` match arm.
- **Minimum wire-up**: `"sacrilege::*" => sacrilege_tools::dispatch(...)` branch.
- **Blocked by**: none.

### `pre-tc-bld-replay` — `bld-replay/src/gameplay_schema.rs` reached by recorder

- **Orphan**: [bld/bld-replay/src/gameplay_schema.rs](../../bld/bld-replay/src/gameplay_schema.rs).
- **Expected call-site**: [bld/bld-replay/src/lib.rs](../../bld/bld-replay/src/lib.rs)
  replay record/playback API — used by P22 W148 determinism test.
- **Minimum wire-up**: `pub use gameplay_schema::*;` + a record function
  that serialises `GameplayEvent` values at tick cadence.
- **Blocked by**: none.

### `pre-tc-ai-retrain-workflow` — `tools/cook/ai/*` attached to CI

- **Orphan**: [tools/cook/ai/director_train/pipeline.yml](../../tools/cook/ai/director_train/pipeline.yml),
  [tools/cook/ai/music_symbolic_train/pipeline.yml](../../tools/cook/ai/music_symbolic_train/pipeline.yml),
  [tools/cook/ai/neural_material](../../tools/cook/ai/neural_material/),
  [tools/cook/ai/vae_weapons](../../tools/cook/ai/vae_weapons/),
  [tools/cook/ai/wfc_rl_scorer](../../tools/cook/ai/wfc_rl_scorer/).
- **Expected call-site**: new `.github/workflows/ai_retrain.yml`
  (nightly-cron retrain stub).
- **Minimum wire-up**: scheduled workflow calling each `pipeline.yml`'s
  entry script; uploads model artifacts; opens tracking issue if model
  hash changes.
- **Blocked by**: none.

---

## Summary table (preflight DAG seed)

| Id                          | Blocked by                          | Execute with                        |
|-----------------------------|-------------------------------------|-------------------------------------|
| `pre-ed-theme-menu`         | —                                   | Track 2                             |
| `pre-ed-sacrilege-panels`   | —                                   | Track 2 (before P21 W140)           |
| `pre-ed-audit-panels`       | —                                   | Track 2                             |
| `pre-ed-module-manifest`    | `pre-ed-sacrilege-panels`, `pre-ed-audit-panels` | Track 2                |
| `pre-ed-a11y-init`          | —                                   | Track 2                             |
| `pre-ed-pie-overlays`       | —                                   | Track 2                             |
| `pre-ed-widgets-firstuse`   | `pre-ed-sacrilege-panels`           | Track 2 (alongside P21 W140)        |
| `pre-eng-ai-runtime-dispatch` | —                                 | Track 2                             |
| `pre-eng-narrative-tick`    | —                                   | Track 2 (before P21 W139)           |
| `pre-eng-franchise-services`| `pre-eng-ai-runtime-dispatch`       | Track 2                             |
| `pre-eng-crash-reporter-init`| —                                  | Track 2                             |
| `pre-eng-steam-input-glue`  | —                                   | Track 2                             |
| `pre-eng-fsr2-hal-frame-graph`| —                                 | Track 2                             |
| `pre-gp-martyrdom-ecs`      | `p22-w143-ecs-components`           | Phase 22                            |
| `pre-gp-acts-tick`          | `p22-w145-blasphemy`                | Phase 22                            |
| `pre-gp-logos-phase4`       | —                                   | Track 2 (stub-wire only)            |
| `pre-gp-malakor-owner`      | `p22-w146-player-controller`        | Phase 22                            |
| `pre-gp-a11y-init`          | `p21-w140-hud-editor-panels`        | Phase 21                            |
| `pre-tc-content-signing`    | —                                   | Track 2                             |
| `pre-tc-fuzz-preset`        | —                                   | Track 2                             |
| `pre-tc-tests-split`        | —                                   | Track 2                             |
| `pre-tc-bld-tool-dispatch`  | —                                   | Track 2                             |
| `pre-tc-bld-replay`         | —                                   | Track 2 (before P22 W148)           |
| `pre-tc-ai-retrain-workflow`| —                                   | Track 2                             |
