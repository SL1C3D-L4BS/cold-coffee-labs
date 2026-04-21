# ADR 0024 — Engine core config: typed CVar registry + TOML persistence

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11B)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0022 §5 (Phase-10 handoff), ADR-0021 (input action map), ADR-0017 (audio mixing), blueprint §3.14

## Context

Phase 10 shipped three TOML files parsed by handwritten line-oriented code:

- `audio_mixing.toml` — bus volumes, reverb presets, downmix mode
- `input.toml` — active action profile, hotplug policy
- `input.$PROFILE.toml` — per-profile binding tables (`action_map_toml.cpp`)

Phase 11 needs a *single* knob system that:

- the **console** can read/write (`get`, `set`);
- the **settings menu** (RmlUi data-model) can read/write;
- the **BLD agent** can read/write over the C-ABI;
- the **editor inspector** can read/write;
- every subsystem can **subscribe** to via `ConfigCVarChanged`;
- round-trips **byte-identically** to the Phase-10 TOML files (so Phase-10 regressions stay green).

Phase 10's line-oriented parser is not extensible enough for nested tables, inline tables, and typed enums; toml++ 3.4.0 is the Greywater-wide pin for TOML going forward.

## Decision

Ship `engine/core/config/` with:

- `CVar<T>` — a typed handle addressing a slot in a global registry. `T ∈ { bool, int32_t, int64_t, float, double, std::string, enum class }`. PODs only in the runtime slot; strings are small-string-optimised via `gw::core::Utf8String`.
- `CVarFlags` bitmask: `kPersist`, `kUserFacing`, `kDevOnly`, `kCheat`, `kReadOnly`, `kRenderThreadSafe`, `kRequiresRestart`, `kHotReload`.
- `CVarRegistry` — singleton per-process. Lock-free read path via a per-thread snapshot; writes funnel through one mutex + a version bump, and publish `ConfigCVarChanged{id, origin}` on the Phase-11A bus.
- `toml_binding.{hpp,cpp}` — `load_domain(path, registry, domain_prefix) → Result<void,Error>` / `save_domain(path, registry, domain_prefix)`. Uses `toml++` when `GW_CONFIG_TOMLPP=ON` (default ON for `gw_runtime`); falls back to the Phase-10 hand-rolled reader otherwise so CI without deps still round-trips.
- `StandardCVars` — ~40 pre-baked CVars seeded at registry construction. Domain prefixes: `gfx.*`, `audio.*`, `input.*`, `ui.*`, `dev.*`, `net.*`.
- **One TOML file per domain**: `graphics.toml`, `audio.toml`, `input.toml`, `ui.toml`. *Not* one monolithic `user.toml` — saves preserve meaning when a human edits one domain.

Origin tagging on `ConfigCVarChanged` (`kProgrammatic`, `kConsole`, `kTOMLLoad`, `kRmlBinding`, `kBLD`) is mandatory so the RmlUi data-model listener skips its own writes and avoids the feedback loop called out in the Phase-11 risks table.

## Phase-10 handoff

Wave 11B takes ownership of Phase-10's TOML files:

- `action_map_toml.cpp` — rewritten to use `toml++` when gated on. The Phase-10 parser stays compiled as the gate-off fallback; the **regression suite** in `unit/input/action_map_test.cpp` is the merge gate — every fixture round-trips byte-identically or the migration fails.
- `audio_mixing.toml` — becomes a `audio.*` CVar domain file. Existing `AudioService::set_bus_volume` calls stay, but subscribe to `ConfigCVarChanged` for propagation.

## Perf contract

| Gate | Target |
|---|---|
| `CVar<T>::get()` | ≤ 5 ns (snapshot load) |
| `CVar<T>::set() + publish` | ≤ 200 ns |
| `load_domain("ui.toml")` cold | ≤ 2 ms for ~40 entries |
| `load_domain("ui.toml")` warm | ≤ 0.3 ms |
| Heap alloc on hot path | 0 B |

## Tests (≥ 12)

1. every scalar type round-trips (`bool, i32, i64, f32, f64, string, enum`)
2. enum serialisation uses named variants (`"hrtf"`, not `3`)
3. persistence across registry rebuild (kill + reload from disk)
4. hot-reload of external edit publishes a second `ConfigCVarChanged`
5. `kReadOnly` enforcement in non-dev builds
6. `kCheat` gating behind `dev.console.allow_cheats`
7. out-of-range clamp on load with diagnostic emitted to log
8. stale-snapshot detection (reader sees old value until fence)
9. concurrent-reader + single-writer race (TSAN clean)
10. TOML parse error reports line + column
11. Phase-10 `input.toml` backward-compat — fixture round-trips byte-identical
12. `action_map_toml.cpp` regression suite still green after toml++ swap

## Alternatives considered

- **JSON / YAML** — JSON is too verbose for hand-editing; YAML is specification-unstable and error-prone on indentation.
- **ini + hand-rolled** — Phase 10's approach; does not scale to nested tables.
- **Binary profile blob** — fine for release ship, hostile to user-editing and diffing.

Rejected. TOML is the community default for game settings; toml++ is header-only, no-exceptions-mode capable, MIT-licensed, and gives us a clean C++23 API.
