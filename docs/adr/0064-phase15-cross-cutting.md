# ADR 0064 — Phase 15 cross-cutting (pins, CI, hand-off)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Supersedes:** —
- **Companion ADRs:** 0056, 0057, 0058, 0059, 0060, 0061, 0062, 0063.

## 1. Dependency pins

| Library | Pin | CMake gate | Default `dev-*` | Default `persist-*` |
|---|---|---|---|---|
| SQLiteCpp | 3.3.1 | existing (always) | ON | ON |
| BLAKE3 (C) | 1.5.4 | always for `.gwsave` footer | ON | ON |
| zstd | 1.5.6 | `GW_ENABLE_ZSTD` | OFF | ON |
| Sentry Native SDK | 0.7.19 | `GW_ENABLE_SENTRY` | OFF | ON |
| cpr | 1.11.1 | `GW_ENABLE_CPR` | OFF | ON |
| Steamworks SDK | Phase 16 | `GW_ENABLE_STEAMWORKS` | OFF | OFF |
| EOS SDK | Phase 16 | `GW_ENABLE_EOS` | OFF | OFF |

Header-quarantine invariant (ADR-0056 §3): none of the above leak into any public header under `engine/persist/*.hpp` or `engine/telemetry/*.hpp`; all vendor types live in `impl/*.cpp`.

## 2. Performance budgets

See `docs/perf/phase15_budgets.md`. The `gw_perf_gate_phase15` binary (tests/perf) enforces the numeric ceilings on every CI build; CTest label `phase15;perf`. Debug builds apply a generous slack so the gate stays green on `dev-*` and tightens under `persist-*` Release.

## 3. Thread ownership

| Owner | Work |
|---|---|
| Main | `PersistWorld` / `TelemetryWorld` lifetime; consent UI FSM |
| Jobs `persist_io` (priority Normal) | `.gwsave` IO, SQLite transactions, migration chain |
| Jobs `telemetry_io` (priority Low) | HTTP flush, Sentry upload, DSAR zip |
| Jobs `background` (priority Background) | cloud sync poll, quota check |

Named lanes are *conventions* on `jobs::Scheduler::submit` priorities — see `engine/jobs/lanes.hpp` (`submit_persist_io`, `submit_telemetry_io`, `submit_background`). No additional thread pools; crashpad's handler is a separate OS process managed by Sentry itself (not a Greywater thread).

## 4. Tick order (runtime)

After `render.extract()` (when present): `persist.step()` then `telemetry.step()`. Neither may block the main thread for IO — both dispatch work onto their named lanes and return within `O(µs)`.

## 5. CI

`.github/workflows/ci.yml` runs:

- `phase15-windows` / `phase15-linux` — `dev-*` preset, CTest `-L phase15` on every commit.
- `phase15-persist-smoke-windows` / `phase15-persist-smoke-linux` — `persist-*` preset with zstd/Sentry/cpr enabled; smoke gates label `phase15|perf`.
- `phase15-persistence-sandbox` — runs `sandbox_persistence`; requires the `PERSIST OK` marker in stdout.
- `phase15-save-compat-regression` — `gw_save_tool validate` + `gw_save_tool migrate --dry-run` over checked-in fixtures under `tests/fixtures/saves/v1/`.
- `phase15-save-cross-platform` — determinism_hash equality check across Windows + Linux; re-uses the parity test cases in `phase15_extras_test.cpp`.

## 6. Exit checklist (retrospective, 2026-04-21)

- [x] ADRs 0056 → 0064 Accepted.
- [x] Zero `<sentry.h>` / `<sqlite3.h>` / `<steam_api.h>` / `<eos_sdk.h>` / `<zstd.h>` in any public header.
- [x] `.gwsave` v1 round-trips ≥ 1024 entities deterministically; `phase15 — identical ECS inputs produce byte-identical gwsave bodies` locks the invariant.
- [x] `sandbox_persistence` prints `PERSIST OK` with 3+ saves, 1+ migration, 2+ cloud round-trips, 500+ events, 1 test-crash, DSAR export.
- [x] 25 `persist.*` / `tele.*` CVars registered; TOML round-trip covered by `phase15 — persist.* + tele.* survive TOML save → load`.
- [x] 12 Phase-15 console commands registered and discoverable; `phase15 — all 12 console commands are registered and discoverable` enforces the invariant.
- [x] ≥ 45 new Phase-15 tests landed across `phase15_sqlite_test.cpp`, `phase15_cloud_test.cpp`, `phase15_persist_world_test.cpp`, `phase15_extras_test.cpp`, `phase15_telemetry_test.cpp`, `persist_gwsave_test.cpp`; `dev-win` CTest count ≥ 445.
- [x] `persist-{win,linux}` presets build + smoke-run green on CI.
- [x] `phase15-persistence-sandbox` + `phase15-save-compat-regression` + cross-platform parity gates enforced.
- [x] `docs/perf/phase15_budgets.md` ceilings enforced by `gw_perf_gate_phase15`.
- [x] `docs/a11y_phase15_selfcheck.md` ticked.
- [x] Roadmap Phase-15 row flipped to `completed` with closeout paragraph (`docs/05_ROADMAP_AND_MILESTONES.md`).

## 7. `.gwsave` v1 format freeze

Layout is frozen as committed in ADR-0056 §4. Any change is a `v1 → v2` migration step in `engine/persist/migration.cpp`; the existing writer + reader continue to support v1 for the lifetime of the engine (determinism ladder §4).

## 8. CVar + console surface (frozen)

25 CVars, 12 commands — exhaustive list in `engine/persist/persist_cvars.cpp` and `engine/persist/console_commands.cpp` + `engine/telemetry/console_commands.cpp`. New entries in Phase 16 may append but must not rename or repurpose existing keys (TOML forward-compat).

## 9. Determinism ladder

1. `.gwsave` writer is bit-deterministic: `phase15 — identical ECS inputs produce byte-identical gwsave bodies`.
2. `determinism_hash` (ADR-0037) embedded in header; loader refuses mismatch in strict mode.
3. BLAKE3-256 footer is content-addressed; no clock, allocator, or RNG state influences it.
4. Migration chain is append-only, pure functions from `SaveReader&` → `SaveWriter&`.
5. SQLite WAL with `synchronous=NORMAL`; `PRAGMA wal_checkpoint(TRUNCATE)` at clean shutdown and before every cloud sync.
6. Telemetry session id = SplitMix64 of `(session_seed XOR user_id_hash)`, reproducible across restart.
7. DSAR zip sorted by key — byte-identical across runs for the same state.
8. Cross-platform parity gate enforces hash equality Windows ↔ Linux.

## 10. Risks retrospective (2026-04-21 closeout)

| Risk | Mitigation shipped | Residual |
|---|---|---|
| Sentry crashpad protocol drift | Pin 0.7.19; bumps gated by ADR; handler path baked in `impl/sentry_backend.cpp` | Phase 16 must rerun the round-trip gate on bump |
| SQLite WAL corruption after power-cut | `journal_mode=WAL`, `synchronous=NORMAL`, truncate-checkpoint at shutdown, `integrity_check` on recovery | Low; ship with telemetry event `persist.integrity_check_failed` |
| Save-format drift across OSes | zstd deterministic CDM, no byte-order runtime detection, `phase15-save-cross-platform` gate CI-mandatory | Nil (gated) |
| COPPA liability for third-party SDKs | Under-13 clamps to `CrashOnly`; `MarketingAllowed` disabled; consent UI disaggregated | Legal must re-verify quarterly per ADR-0062 |
| Steam Cloud conflict opacity | `PreferNewer` + vector clock = 95 % deterministic; `Prompt` fallback surfaces Steam-native UI | Phase 16 wires real Steam backend |
| EOS quota surprise | `QuotaWarning` at 80 % (CVar-configurable), hard block at 100 %, per-put poll | Phase 16 wires real EOS |
| Crashpad failure on sandboxed Linux distros | Fallback to in-process `sentry_capture_event`, local minidump retention | Phase 16 persist-linux preset smoke |
| Save bloat from uncompressed chunks | zstd-3 default behind `persist.compress.enabled`; per-chunk compression | None in gate |
| DSAR PII on disk post-export | User picks dir; README recommends BitLocker/LUKS; export is sorted + deterministic | User-education dependent |
| Telemetry endpoint outage | SQLite queue with exp-backoff; `tele.queue.max_age_days` drops per GDPR minimisation | Phase 16 plugs real cpr backend |
| BLD session hallucinating consent change | `tele.consent.*` CVars are `Execute`-tier (ADR-0015) — BLD cannot flip without user form-mode | Periodic audit |
| Compliance drift | ADR-0062 is the amendment log; quarterly BLD check-in required | Process discipline |

## 11. Hand-off to Phase 16 (*Ship Ready*, weeks 102–107)

Phase 16 inherits three frozen contracts:

1. **`ICloudSave` frozen** — Phase-16 Steamworks + EOS waves plug production impls into existing stubs without interface churn. `CloudConflictPolicy` enum is append-only.
2. **`ITelemetrySink` + consent tiers frozen** — Phase-16 WCAG 2.2 AA audit (week 107) validates RmlUi consent + DSAR UX without backend changes.
3. **`.gwsave` v1 frozen** — Phase-16 locale work adds display-string rows to slot metadata; any schema change is a `v1 → v2` migration, never a format break.

The `persist-*` preset becomes the base for the `ship-*` preset.

## 12. Exit gate closeout

Phase 15 exits Accepted on 2026-04-21. Hand-off block above is normative for Phase 16. For the roadmap status flip see `docs/05_ROADMAP_AND_MILESTONES.md` §Phase-15 closeout.
