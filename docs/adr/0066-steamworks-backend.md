# ADR 0066 — Steamworks backend (`GW_ENABLE_STEAMWORKS`)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16B)
- **Pin:** Steamworks SDK **1.64** (shipped 2026-Q1).
- **Companions:** 0065 (façade), 0059 (`ICloudSave` freeze), 0073.

## 1. Decision

Integrate Steamworks SDK 1.64 behind the `GW_ENABLE_STEAMWORKS` CMake gate.
The only TUs that include `<steam_api.h>` are `impl/steam_backend.cpp` and
`impl/steam_cloud_save.cpp`; public headers remain SDK-free (verified by the
header-quarantine test).

## 2. Surface implemented

1. `make_steamworks_platform_services(cfg)` — returns `nullptr` if
   `SteamAPI_Init()` fails or if `dry_run_sdk` is true.
2. `make_steamworks_cloud_save(cfg)` — plugs the `ICloudSave` factory
   without ADR-0059 interface churn. Steam Cloud quota goes through
   `ISteamRemoteStorage::GetQuota`.
3. Workshop — the `list_subscribed_content()` and `download(id)` calls
   shim through `ISteamUGC`.
4. Achievements/stats — `ISteamUserStats`; `store_stats()` is called on
   `step()` when dirty.
5. Rich presence — `ISteamFriends::SetRichPresence(key, localized_value)`.

## 3. Lifecycle

- `initialize()` calls `SteamAPI_Init()` once; failure returns
  `PlatformError::BackendDisabled`; subsequent calls are no-ops.
- `step()` pumps `SteamAPI_RunCallbacks()` and drains the leaderboard
  submission queue (max 4 submissions per frame to stay inside the SDK's
  2/sec implicit cap).
- `shutdown()` calls `SteamAPI_Shutdown()`.

## 4. Rate-limiting + re-entry

The façade ring-buffer (`plat.rate_limit_per_minute`) gates every SDK
call so Steam's IPC queue never saturates. Re-entry is blocked by an
internal mutex — callbacks from Steam fire on a helper thread and go
through a single-producer `MPSCQueue<Event>` before reaching façade
consumers.

## 5. CI shape

- `ship-windows` / `ship-linux` presets define `GW_ENABLE_STEAMWORKS=ON`
  (Phase 16 closeout). The CI job runs a **signed** smoke that uses the
  real AppID 480 (Spacewar) sandbox with `dry_run_sdk=true`; production
  keys are never checked in.
- `dev-*` presets keep the gate OFF → all tests link against the
  dev-local backend only.

## 6. Header quarantine

```
engine/platform_services/impl/
  steam_backend.hpp       # forward decls only
  steam_backend.cpp       # includes <steam_api.h>
  steam_cloud_save.cpp    # includes <steam_api.h>
  steam_ugc.cpp           # includes <steam_api.h>
```

A CI grep for `#include <steam_api` outside `impl/` fails the build.

## 7. Risks

| Risk | Mitigation |
|---|---|
| SDK 1.64 header re-organisation (`steam_gameserver.h` split) | Only include `steam_api.h`; never pin internal headers |
| `SteamAPI_Init` order with GNS (Phase 14) | GNS' `SteamNetworkingSockets::Init()` is independent; wired before façade init |
| Workshop quota surprise | `UGCQueryHandle_t` is paged; default page 20 items |
| Steam overlay pause during `step()` | Façade `step()` is idempotent + wall-clock budget 0.5 ms |
