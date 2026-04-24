# Editor mandate — Sacrilege-first (internal)

**Status:** Working spec · ties GW-SAC-001 + `COMPOSER_CONTEXT.md` to editor UX.

## Scope

- The Greywater Editor exists to ship **Sacrilege** — not a generic engine demo. Surfaces, defaults, and copy assume **one franchise**, **Nine Circles**, **Hell Seed**, **Blacklake** procedural arenas (`docs/08_BLACKLAKE.md`).
- **Gameplay code** remains **C++23 only** (`COMPOSER_CONTEXT.md`). Editor tooling may use offline `tools/` (Python, etc.) for manifests and cooks.

## Chrome vs shipped game

- **Editor chrome:** Cold Drip × Sacrilege — charcoal panels, controlled accents, Corrupted Relic / high-contrast themes (`docs/03_PHILOSOPHY_AND_ENGINEERING.md`). This is **not** the in-game look.
- **Shipped levels:** GW-SAC-001 §14 — crunchy low-poly horror, **256–512** texture doctrine where applicable, circle palettes for readability. **AmbientCG CC0** may be look-dev only per `docs/adr/ADR-0120-ambientcg-cc0-shipping.md`.

## Required authoring surfaces

- **Sacrilege Library** — indexed materials, textures, CC0 baseline (baked manifest + live project).
- **Ten Sacrilege panels** — see `docs/prompts/preflight/pre-ed-sacrilege-panels.md`; `PanelRegistry::find` must resolve each registered name.
- **Readiness** — hub / panel traffic lights for manifests, optional default scene, cook hints (`docs/BOOTSTRAP_ASSETS_WIRING.md`).

## Blacklake / Hell Seed

- **Hell Seed** and **circle profile** are deterministic inputs to arena generation. The shell should surface stub context (status bar / Circle Editor) until full Blacklake preview lands.
- **Designer blockout** (Phase 1 primitives) must remain distinguishable from **GPTM-generated** mesh; regen must not delete blockout unless explicitly opted in (`docs/LVL_BLOCKOUT_RESEARCH.md`).

## References

- `docs/07_SACRILEGE.md` — GW-SAC-001
- `docs/COMPOSER_CONTEXT.md` — session cold start
- `docs/FRAMEWORKS_GREYWATER_MAPPING.md` — Unity / Unreal / Godot parity table
