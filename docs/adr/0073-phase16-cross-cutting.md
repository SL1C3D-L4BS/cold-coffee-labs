# ADR 0073 — Phase 16 cross-cutting (pins, CI, hand-off)

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 exit
- **Companions:** 0065, 0066, 0067, 0068, 0069, 0070, 0071, 0072.

## 1. Dependency pins

| Library | Pin | CMake gate | `dev-*` | `ship-*` |
|---|---|---|---|---|
| Steamworks SDK | 1.64 | `GW_ENABLE_STEAMWORKS` | OFF | ON |
| EOS SDK | 1.17 | `GW_ENABLE_EOS` | OFF | ON |
| ICU4C | 78.3 (Unicode 17 / CLDR 48) | `GW_ENABLE_ICU` | OFF | ON |
| HarfBuzz | 14.0.0 | `GW_ENABLE_HARFBUZZ` | OFF (but `playable-*` ON) | ON |
| AccessKit-C | 0.14.0 | `GW_ENABLE_ACCESSKIT` | OFF | ON |

Header-quarantine invariant: none of the above leak into any public
header under `engine/platform_services/*.hpp`, `engine/i18n/*.hpp`, or
`engine/a11y/*.hpp`. `tests/integration/phase16_header_quarantine_test.cpp`
enforces.

## 2. Performance budgets

See `docs/perf/phase16_budgets.md`. `gw_perf_gate_phase16` enforces:

| metric | null `dev-*` | `ship-*` |
|---|---|---|
| `PlatformServicesWorld::step()` | ≤ 100 µs | ≤ 500 µs |
| `bidi_segments` (256 scalars) | ≤ 2 µs | ≤ 8 µs |
| `format_message` | ≤ 5 µs | ≤ 20 µs |
| `gwstr` load (10 k keys) | ≤ 300 µs | ≤ 1 ms |
| a11y `selfcheck` | ≤ 50 µs | ≤ 200 µs |

## 3. Thread ownership

| Owner | Work |
|---|---|
| Main | `PlatformServicesWorld` lifecycle; consent UI; a11y tree update |
| Jobs `platform_io` (Normal) | Workshop/UGC downloads, leaderboard submissions |
| Jobs `i18n_io` (Normal) | `.gwstr` load, XLIFF re-read on hot-reload |
| Jobs `background` (Background) | Presence refresh, friends count poll |

The names match ADR-0064 §3 lanes; `jobs::submit_platform_io`,
`jobs::submit_i18n_io` become available from `engine/jobs/lanes.hpp`.

## 4. Tick order

After Phase-15 `persist.step()` + `telemetry.step()`:
- `platform_services.step()` pumps SDK callbacks (dev-local: no-op),
- `i18n.step()` services XLIFF hot-reload (dev-local: no-op),
- `a11y.step()` rebuilds accessibility tree deltas,
- subtitles queue drain inside `ui.render()` (no additional tick step).

No subsystem may block main > 0.5 ms in `ship-*`.

## 5. CI

`.github/workflows/ci.yml`:

- `phase16-windows` / `phase16-linux` — `dev-*`, CTest `-L phase16`.
- `phase16-platform-smoke-windows` / `phase16-platform-smoke-linux` —
  `ship-*` preset with Steam + EOS ON in `dry_run_sdk=true` mode; smoke
  runs `sandbox_platform_services` and verifies `SHIP READY` marker.
- `phase16-locales` — validates every `.xliff` has ≥ 95% of canonical
  key set for each of {en-US, fr-FR, de-DE, ja-JP, zh-CN}.
- `phase16-a11y-selfcheck` — boots `sandbox_platform_services`, dumps
  `a11y_selfcheck.json`, fails on any `wcag22.*=Fail`.
- `phase16-header-quarantine` — greps for forbidden includes outside
  `impl/`; fails on any hit.

## 6. Exit checklist

- [x] ADRs 0065 → 0073 Accepted.
- [x] Zero `<steam_api.h>` / `<eos_sdk.h>` / `<unicode/*.h>` / `<hb.h>` /
      `<accesskit.h>` in any public header.
- [x] `PlatformServicesWorld` PIMPL ships with dev-local backend + Steam
      + EOS stubs compile-gated.
- [x] `.gwstr` v1 round-trips bit-deterministically across Windows +
      Linux.
- [x] `LocaleBridge` consumes `.gwstr`; ICU null + real shims both pass
      the plural + date tests.
- [x] BiDi segments are stable across Windows + Linux for the canonical
      `ar`, `he`, and mixed-Latin/Arabic fixtures.
- [x] MessageFormat 2 null shim resolves `{count, plural, one {.}
      other {.}}` + `{name}` correctly.
- [x] HarfBuzz `GW_ENABLE_HARFBUZZ=ON` smoke (part of `playable-*` +
      `ship-*`) shapes a Latin string ≤ 50 µs.
- [x] a11y modes + subtitles + screen-reader bridge ship with null
      backend; `accesskit` compiles gated.
- [x] ≥ 86 new Phase-16 tests landed; `dev-win` CTest count ≥ 559.
- [x] `ship-{win,linux}` presets build + smoke-run green on CI.
- [x] `sandbox_platform_services` prints `SHIP READY` with `steam=✓
      eos=✓ wcag22=AA i18n_locales≥5 a11y_selfcheck=green`.
- [x] `docs/perf/phase16_budgets.md` ceilings enforced.
- [x] `docs/a11y_phase16_selfcheck.md` ticked.
- [x] Roadmap Phase-16 row flipped to `completed`.

## 7. Frozen contracts handed to Phase 17

1. **`IPlatformServices` frozen** — Phase-17 shader/VFX work does not
   touch this surface; Phase-18 mods reuse `list_subscribed_content`.
2. **`.gwstr` v1 frozen** — cinematics (Phase 18) author localized
   subtitle cues against the same table.
3. **`a11y::SelfCheckReport` frozen** — release engineering (Phase 24)
   ratifies it as the WCAG 2.2 AA checklist.

## 8. Risks retrospective

| Risk | Mitigation shipped | Residual |
|---|---|---|
| Steamworks / EOS dual-init ordering | Façade owns both as deterministic vector; `SteamAPI_Init` before `EOS_Initialize` | Low; gated by `ship-*` CI |
| AccessKit-C crashes on headless Linux | Null backend is the default; AccessKit load is try/catch in `impl/` | AT-SPI bus absence surfaces warning only |
| ICU 78 plural rules drift from CLDR 47 | Lockfile pins CLDR 48 tables; null shim mirrors English | Locale coverage CI gate detects regressions |
| HarfBuzz 14 SDF baseline vs Phase-11 contract | Public `ShapedGlyph` POD unchanged; advance encoding stays 26.6 fixed | Null build's 1:1 glyph mapping remains the test oracle |
| WCAG 2.2 §2.4.11 focus obscuring | `a11y.focus.show_ring` defaults ON; selfcheck flips Fail when hidden | Manual audit still required per release |
| Translator burden of 5 locales | XLIFF round-trip + MessageFormat 2 lint | Procurement concern, not technical |
| Photosensitivity false-negatives | Three-flash rule clamps audio + UI; scene artists must annotate | Phase 18 VFX doc captures |
| Steam + EOS achievement drift | Dual-unlock via cross-subsystem bus; idempotent on both backends | Manual QA pass per milestone |
| Rich-presence PII leak | `rich_presence` values are `StringId`s (pre-localised); free text rejected | Enforced by `IPlatformServices::set_rich_presence` contract |
| `.gwstr` content-hash drift by OS line-endings | XLIFF cook normalises to LF + NFC before hashing | CI parity gate (Windows ↔ Linux) |
| WCAG 2.2 §1.4.4 text scaling vs layout break | Settings binder clamps `ui.text.scale` ∈ [0.5, 2.5]; UI reflow tested | Phase 18 cinematics must retest |
| BLD session drifting a11y CVars | `a11y.*` CVars registered as `Execute` tier (ADR-0015) — BLD cannot flip without form-mode elicitation | Periodic audit |
