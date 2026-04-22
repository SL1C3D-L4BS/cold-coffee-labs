# ADR 0067 — Epic Online Services backend (`GW_ENABLE_EOS`)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16C)
- **Pin:** EOS SDK **1.17** (2026-Q1).
- **Companions:** 0065 (façade), 0066 (Steam companion), 0073.

## 1. Decision

Integrate EOS SDK 1.17 behind the `GW_ENABLE_EOS` CMake gate. Same shape as
ADR-0066: `impl/eos_backend.cpp`, `impl/eos_cloud_save.cpp`,
`impl/eos_achievements.cpp`. No public header pulls `<eos_sdk.h>`.

## 2. Surface implemented

1. `make_eos_platform_services(cfg)` — returns `nullptr` on
   `EOS_Platform_Create` failure or if `dry_run_sdk`.
2. `make_eos_cloud_save(cfg)` — plugs into the ADR-0059 factory.
3. Achievements via `EOS_Achievements` interface; mirrors stats through
   `EOS_Stats_IngestStat`.
4. Leaderboards via `EOS_Leaderboards_QueryLeaderboardRanks`.
5. Rich presence via `EOS_Presence_SetPresence`.
6. Mod/UGC feed via **Epic Mod SDK** (`EOS_ModSdk` 1.17).

## 3. Lifecycle

- `EOS_Initialize` + `EOS_Platform_Create` in `initialize`.
- `step()` pumps `EOS_Platform_Tick` and drains the submit queue.
- `shutdown()` calls `EOS_Platform_Release` then `EOS_Shutdown`.

## 4. Parity with Steam

Dual-unlock is achieved by emitting `platform.achievement.unlocked` on the
cross-subsystem bus; the Steamworks + EOS backends both listen and call
their respective SDK once. This is what the *Ship Ready* milestone gate
means by "Steam + EOS dual-unlock from a single gameplay event."

## 5. Risks

| Risk | Mitigation |
|---|---|
| EOS SDK dual-init with Steamworks | Both can run in the same process; the façade owns them in a deterministic vector |
| EOS Cloud conflict semantics differ from Steam | The ADR-0059 `CloudConflictPolicy` enum is append-only; EOS backend maps its ETag to `vector_clock` |
| Anti-cheat surface (Easy Anti-Cheat) | Out of scope for Phase 16; Phase 24 reopens |
| Epic Account login flow vs Steam offline | Façade `signed_in()` surfaces false when EOS auth has not completed; games degrade gracefully |
