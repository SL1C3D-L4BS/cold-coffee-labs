# ADR 0028 — Text shaping pipeline: FreeType 2.14.3 + HarfBuzz 14.1.0 + SDF atlas + locale bridge

- **Status:** Accepted
- **Date:** 2026-04-21 (Phase 11 Wave 11D)
- **Deciders:** Cold Coffee Labs engine group
- **Related:** ADR-0026 (UI service), blueprint §3.20

## Context

RmlUi ships with a bundled FreeType font provider, but for the game we need:

- **SDF glyphs** so `ui.text.scale 0.75×–2.0×` does not re-shape per frame.
- **COLRv1 colour emoji** in captions and chat.
- **CJK + complex-script shaping** so the Japanese locale test passes on day one.
- A **locale bridge** that Phase 16 can replace with ICU without breaking callers.

## Decision

Ship `engine/ui/` with:

- `FontLibrary` — owns a thread-safe cache of `FT_Face` objects keyed by font-id. Faces are instantiated once (`FT_New_Memory_Face`); per-size shapers are cheap derivatives.
- `TextShaper` — wraps `hb_shape` against the FT face. Inputs: script, direction, language tag, a span of UTF-8 code points. Output: `ShapedRun { std::span<GlyphInstance> }` where `GlyphInstance = { glyph_id, x_advance, x_offset, y_offset, cluster }`. Returned spans are backed by a per-shape-call arena (`gw::memory::arena_allocator`); callers copy if they need persistence.
- `GlyphAtlas` — 4 K × 4 K `R8_UNORM` signed-distance-field atlas rasterised via FreeType's `FT_RENDER_MODE_SDF` (introduced in 2.10, stabilised in 2.14). Pages are uploaded on demand through the HAL; eviction policy is LRU per locale-group. Fallback alpha-coverage path for bitmap emoji (COLRv1 / CBDT) via HarfBuzz's `hb_color` API and FT's SVG driver.
- `LocaleBridge` — a simple `StringId → std::string_view` map. Phase 11 ships a TOML-backed placeholder loader (`assets/strings/en-US.toml`, `fr-FR.toml`, `ja-JP.toml`). The interface is frozen so ADR-0044 (Phase 16) can plug ICU + MessageFormat behind it.

### Dependency pins

| Library | Pin | Gate | Why |
|---|---|---|---|
| FreeType | 2.14.3 (2026-03-22) | `GW_UI_FREETYPE` (default ON in `gw_runtime`, OFF in CI) | 2.14 adds dynamic HarfBuzz loading; 2.14.3 patches two CVEs and improves LCD filtering. SDF driver via `FT_CONFIG_OPTION_SDF`. |
| HarfBuzz | 14.1.0 (2026-04-04) | `GW_UI_HARFBUZZ` | 14.x stabilises COLRv1 paint-tree and ships the Slug GPU rasteriser; we consume CPU-only but keep the 14 API for future Slug adoption. |

When either gate is OFF, a placeholder shaper emits monospace-Latin-only runs and a dummy atlas of empty glyphs — enough to compile + run; tests marked `[skip-if-no-fonts]` skip.

### Threading

- `FontLibrary` face-cache mutex-guarded (coarse-grained; N ≤ 16 fonts in a running game).
- `TextShaper` is stateless per call; many can run concurrently.
- `GlyphAtlas` upload path runs on the I/O job pool; rasterisation (`FT_Render_Glyph` with SDF) dispatched to the jobs pool for pages under hot-load.

## Perf contract

| Gate | Target |
|---|---|
| Shape + layout 200-glyph Latin run | ≤ 0.2 ms |
| Shape + layout 100-glyph CJK run | ≤ 0.4 ms |
| Shape + layout 50-glyph Arabic (RTL) | ≤ 0.5 ms |
| Glyph atlas upload delta / frame | ≤ 64 KB steady, ≤ 512 KB worst |
| COLRv1 emoji raster | ≤ 0.1 ms per 72×72 glyph |

## Tests (≥ 8 in this ADR)

1. Latin shaping vs. pinned HarfBuzz oracle corpus
2. CJK shaping against pinned HB fixture (Noto Sans CJK JP)
3. RTL shaping correctness with a right-to-left Arabic fixture (even though RmlUi RTL layout is Phase 16, HB output must already be correct)
4. COLRv1 glyph raster round-trip matches bitmap oracle within 1 ULP
5. SDF atlas eviction under hot-load (1000 random glyphs, cache size = 512)
6. `ui.text.scale = 1.5` preserves layout metrics without re-shaping
7. LocaleBridge resolves `StringId → UTF-8` for en-US / fr-FR / ja-JP fixtures
8. Missing glyph falls back to `.notdef` rather than crashing

## Alternatives considered

- **MSDF instead of SDF** — higher quality at small sizes but needs a preprocessing step; we don't ship a tool for it in Phase 11. SDF is sufficient for our target scale range.
- **RmlUi's bundled FT font provider** — doesn't expose SDF or HarfBuzz path; replacing it is cleaner than forking.
- **Slug (HarfBuzz 14 GPU rasteriser)** — requires fp64 or indirect pixel-shader work that the RX 580 can run but we haven't validated. Defer to Phase 17; consume CPU HB now.
- **ICU now** — 50+ MB binary, full i18n surface, out of scope for Phase 11 motor budget. `LocaleBridge` seam reserves the slot.

## Consequences

- `assets/ui/fonts/` ships three fonts by default: **Inter** (Latin UI), **Noto Sans CJK** (CJK fallback), **Noto Color Emoji** (COLRv1 emoji). Licences: Inter is OFL 1.1; Noto is OFL 1.1; Noto Color Emoji is OFL 1.1 (Google SIL OFL release). All three are redistributable under the repo's license manifest.
- Phase 16's ICU drop replaces `LocaleBridge` internals, keeping the interface.
