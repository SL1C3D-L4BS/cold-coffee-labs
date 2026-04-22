# ADR 0070 — HarfBuzz shaping, ICU BiDi, MessageFormat 2

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16D)
- **Pins:** HarfBuzz **14.0.0** (2026-Q1) · ICU4C 78.3 (shared with ADR-0069).
- **Companions:** 0028 (Phase 11 shaper), 0068, 0069, 0071 (a11y bundle).

## 1. Decision

Promote the Phase-11 `TextShaper` null mapper to a real HarfBuzz 14.0.0
shaper behind `GW_ENABLE_HARFBUZZ` (already wired for `playable-*`).
Phase 16 adds three new capabilities:

1. **Per-locale font-stack fallback.** `FontLibrary::fallback_chain(bcp47)`
   resolves a fallback order per locale (e.g. `ja-JP` → `Noto Sans CJK JP` →
   `Noto Sans` → emoji), so a single shape call can span scripts.
2. **ICU BiDi segmentation (UBA 15).** `gw::i18n::bidi_segments(utf8,
   base_dir)` returns runs of `TextDirection`; the shaper then calls HarfBuzz
   once per run with the right direction + script tag. Null builds return a
   single LTR run (stable for tests).
3. **MessageFormat 2** (ICU 78 final) — `gw::i18n::format_message(tag,
   pattern, args)` returns a localized string with plural / selectordinal /
   number / date placeholders. Null build implements a deterministic subset
   (`{name}` + `{count, plural, one {X} other {Y}}` + `{n, number}`).

## 2. Interface delta

`engine/i18n/bidi.hpp`:
```cpp
namespace gw::i18n {

enum class BaseDirection : std::uint8_t { LTR, RTL, Auto };

struct BidiRun {
    std::uint32_t start;  // byte offset
    std::uint32_t size;   // bytes
    ui::TextDirection dir;
    ui::TextScript    script;
};

std::size_t bidi_segments(std::string_view utf8, BaseDirection base,
                           std::vector<BidiRun>& out);

} // namespace gw::i18n
```

`engine/i18n/message_format.hpp`:
```cpp
namespace gw::i18n {

struct FmtArg {
    std::string_view key;
    enum class Kind { Str, I64, F64 } kind;
    std::string_view s;
    std::int64_t     i;
    double           f;
};

std::string format_message(std::string_view bcp47,
                           std::string_view pattern,
                           std::span<const FmtArg> args);

} // namespace gw::i18n
```

## 3. Shaper delta

`TextShaper::shape()` splits input into BiDi runs and shapes each run
with HarfBuzz when `GW_ENABLE_HARFBUZZ` is on. Null build returns the
Phase-11 contract unchanged (one glyph per codepoint, advance_x ≈
`pixel_size * 0.6`).

## 4. Performance budgets

- `bidi_segments(len ≤ 256 scalars)` < 2 µs on null; < 8 µs on ICU.
- `shape(256 scalars, Latin)` < 50 µs on HarfBuzz 14.
- `format_message` < 5 µs null; < 20 µs ICU.

Enforced by `gw_perf_gate_phase16` (ADR-0073 §6).

## 5. Header quarantine

`<hb.h>` only in `impl/hb_shaper.cpp`. `<unicode/ubidi.h>` only in
`impl/icu_bidi.cpp`. Public headers stay SDK-free.
