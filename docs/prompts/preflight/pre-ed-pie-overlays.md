# pre-ed-pie-overlays - PIE debug HUD / perf guard / rollback inspector

<!-- Rendered from docs/prompts/pack.yml - do NOT edit by hand. -->

| Field | Value |
|-------|-------|
| Kind | `preflight` |
| Tier | A |
| Depends on | *(none)* |

## Inputs

- `editor/pie/pie_debug_hud.hpp`
- `editor/pie/pie_perf_guard.hpp`
- `editor/pie/rollback_inspector.hpp`
- `editor/pie/gameplay_host.cpp`

## Writes

- `editor/pie/gameplay_host.hpp`
- `editor/pie/gameplay_host.cpp`

## Exit criteria

- [ ] HUD renders when in_play() and overlays don't crash on pause/resume
- [ ] Perf guard logs a warning when tick > budget

## Prompt

Own one instance each of PieDebugHud, PiePerfGuard,
RollbackInspector on `GameplayHost`. Call HUD::render() and
PerfGuard::sample() from tick(); RollbackInspector::capture() from
stop().

