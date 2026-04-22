# ADR 0065 — `PlatformServicesWorld` facade + `IPlatformServices`

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16A)
- **Companions:** 0066 (Steamworks), 0067 (EOS), 0059 (cloud-save freeze), 0073 (phase-cross-cutting).

## 1. Context

Phase 15 froze `ICloudSave`. Phase 16 needs a generic store-facing façade that exposes:
- user identity (provider id, display name, stable user-hash),
- achievements + stats,
- leaderboards (read + submit),
- rich-presence strings (locale-bound — binds to Phase 16 i18n),
- friends presence (count only, never PII),
- user-generated content (Workshop / Mods feed),
- rate-limited events (per-backend throttle).

The **exact same interface** must admit the Phase-15 `ICloudSave` splice, the
Phase-16B Steamworks backend, and the Phase-16C EOS backend, without any
leakage of `<steam_api.h>` or `<eos_sdk.h>` into public headers.

## 2. Decision

Ship `engine/platform_services/platform_services_world.{hpp,cpp}` as a PIMPL
façade that owns an `IPlatformServices` pointer plus a separate `ICloudSave`
adapter (splice target for ADR-0059). Public headers never include any third-
party SDK header.

`IPlatformServices` is a pure virtual interface with these groups:

1. **Identity** — `current_user()`, `signed_in()`, `user_id_hash()`.
2. **Achievements** — `unlock_achievement(id)`, `is_unlocked(id)`, `clear(id)`.
3. **Stats** — `get_stat_i32/f32`, `set_stat_i32/f32`, `store_stats()`.
4. **Leaderboards** — `submit_score(board, value)`, `top_scores(board, k, out)`.
5. **Rich presence** — `set_rich_presence(key, value_loc_id)`.
6. **UGC / Workshop** — `list_subscribed_content(out)`, `download(content_id)`.
7. **Events** — `publish_event(name, payload_json)` (respects `tele.consent`).
8. **Lifecycle** — `backend_name()`, `initialize(cfg)`, `step(dt)`, `shutdown()`.

Three concrete backends ship this phase:

- **`make_dev_local_platform_services`** (16A) — in-memory, SQLite-backed,
  always available; used by `dev-*` + every unit test.
- **`make_steamworks_platform_services`** (16B) — compile-gated on
  `GW_ENABLE_STEAMWORKS`; `impl/steam_backend.cpp` is the only TU that
  includes `<steam_api.h>`.
- **`make_eos_platform_services`** (16C) — compile-gated on
  `GW_ENABLE_EOS`; `impl/eos_backend.cpp` is the only TU that includes
  `<eos_sdk.h>`.

A **factory** `make_platform_services_aggregated(backend, cfg)` returns the
dev-local backend unless the requested SDK is compiled in and available, in
which case it returns that backend. This mirrors the Phase-15 cloud-save
factory exactly.

## 3. Header-quarantine invariant

`grep -r '#include <steam_api' engine/platform_services/ | grep -v impl/`
produces zero hits. Same for `<eos_sdk`. Enforced in CI by
`tests/integration/phase16_header_quarantine_test.cpp`.

## 4. Interface (abbreviated — full in code)

```cpp
namespace gw::platform_services {

struct UserRef {
    std::string   id_hash_hex;
    std::string   display_name;
    std::string   locale_bcp47;
};

enum class PlatformError : std::uint8_t {
    Ok = 0, NotSignedIn, NotFound, RateLimited, BackendDisabled, IoError,
};

class IPlatformServices {
public:
    virtual ~IPlatformServices() = default;

    [[nodiscard]] virtual UserRef current_user() const = 0;
    [[nodiscard]] virtual bool signed_in() const noexcept = 0;

    virtual PlatformError unlock_achievement(std::string_view id) = 0;
    [[nodiscard]] virtual bool is_unlocked(std::string_view id) const = 0;

    virtual PlatformError set_stat_i32(std::string_view key, std::int32_t v) = 0;
    virtual PlatformError submit_score(std::string_view board, std::int64_t v) = 0;

    virtual PlatformError set_rich_presence(std::string_view key,
                                            std::string_view value_utf8) = 0;

    virtual PlatformError publish_event(std::string_view name,
                                        std::string_view payload_json) = 0;

    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;

    virtual void step(double dt_seconds) = 0;
};

struct PlatformServicesConfig {
    std::string  backend{"local"};    // "local" | "steam" | "eos"
    std::string  storage_dir;         // expanded by the world (ADR-0056 `$user`)
    std::string  app_id;              // Steam AppID / EOS ProductId (ignored by local)
    bool         dry_run_sdk{true};   // in CI: short-circuit real SDK calls
};

[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_dev_local_platform_services(const PlatformServicesConfig&);

[[nodiscard]] std::unique_ptr<IPlatformServices>
    make_platform_services_aggregated(std::string_view backend,
                                      const PlatformServicesConfig&);

} // namespace gw::platform_services
```

## 5. CVars

Seven new `plat.*` CVars:

| key | default | notes |
|---|---|---|
| `plat.backend` | `"local"` | `local` / `steam` / `eos` |
| `plat.app_id` | `""` | Steam AppID / EOS ProductId |
| `plat.achievements.enabled` | `true` | |
| `plat.leaderboards.enabled` | `true` | |
| `plat.rich_presence.enabled` | `true` | |
| `plat.rate_limit_per_minute` | `120` | per event name |
| `plat.dry_run_sdk` | `true` | CI shortcut; never hits real SDK network |

## 6. Consequences

- Steam/EOS backends stay purely additive — no interface churn in Phases
  17+. The `ICloudSave` freeze from ADR-0064 §11 binds; Phase 16B only
  provides a concrete `make_steamworks_cloud_save()` that slots into the
  existing factory.
- Rate-limiting is backend-agnostic; a ring-buffer in the façade catches
  abuse before any SDK call. `plat.rate_limit_per_minute` is the knob.
- The `sandbox_platform_services` exit gate uses the dev-local backend —
  real SDK runs stay behind `ship-*` preset + signed CI job.
