# `tests/split/` — Per-phase test binary split (ADR-0117)

Part C §25 scaffold. ADR-0117 resolves to break the monolithic `gw_tests`
binary into per-phase executables (`gw_tests_phase17`, `gw_tests_physics`,
`gw_tests_net`, etc.) so CI shards can run in parallel and failures attribute
to a phase owner.

This folder seeds the scaffolding. Each `gw_tests_<slug>.cmake` file is a
drop-in that a future migration can include from `tests/CMakeLists.txt`,
replacing the corresponding sources from the monolith. The migration is
intentionally staged — do not delete sources from the monolith until the
matching split target runs green on both dev presets.

## Rollout order (ADR-0117)

1. `gw_tests_physics`  — largest and most stable subsystem.
2. `gw_tests_net`      — natural determinism boundary.
3. `gw_tests_ai`       — new runtime AI (Phase 26).
4. `gw_tests_render`   — bundles Phase 17 + frame graph + post stack.
5. Remainder migrated phase-by-phase.

## CI

Once each split target lands, `ctest -L phaseNN` maps cleanly to a sharded
matrix job. The `determinism.yml` and `fuzz.yml` workflows added alongside
this ADR already consume the split label scheme.
