# ADR 0069 — ICU runtime + XLIFF 2.1 workflow

- **Status:** Accepted
- **Date:** 2026-04-21
- **Phase:** 16 (wave 16C)
- **Pin:** ICU4C **78.3** (Unicode 17 / CLDR 48, 2026-Q1).
- **Companions:** 0068 (`.gwstr`), 0070 (shaping + MessageFormat 2).

## 1. Decision

Link ICU4C 78.3 behind `GW_ENABLE_ICU` for number/date/plural/case
services. The `dev-*` preset builds a **null ICU shim** (`icu_null.cpp`)
that implements the same C++ interface using deterministic fall-backs
(English plurals, C-locale numbers, ISO-8601 dates). This keeps Phase
16 tests deterministic and the CI matrix CPU-light.

## 2. Public interface (sketch)

```cpp
namespace gw::i18n {

class NumberFormat {
public:
    static std::unique_ptr<NumberFormat> make(std::string_view bcp47);
    virtual ~NumberFormat() = default;

    virtual std::string format(std::int64_t v) const = 0;
    virtual std::string format(double v, int frac_digits) const = 0;
};

class DateTimeFormat {
public:
    static std::unique_ptr<DateTimeFormat> make(std::string_view bcp47,
                                                std::string_view skeleton);
    virtual ~DateTimeFormat() = default;

    virtual std::string format(std::int64_t unix_ms) const = 0;
};

enum class PluralCategory : std::uint8_t { Zero, One, Two, Few, Many, Other };
[[nodiscard]] PluralCategory plural_category(std::string_view bcp47,
                                              double quantity) noexcept;

[[nodiscard]] std::string to_lower_locale(std::string_view utf8, std::string_view bcp47);
[[nodiscard]] std::string to_upper_locale(std::string_view utf8, std::string_view bcp47);

} // namespace gw::i18n
```

## 3. XLIFF 2.1 workflow

Source of truth for translators: `content/locales/*.xliff` (XLIFF 2.1 with
ICU MessageFormat 2 placeholders). Cook pipeline:

1. `gw_cook locale scan` reads `.xliff`, validates MessageFormat 2
   placeholders, and flags missing translations against the English
   baseline.
2. Per-locale `*.gwstr` is emitted (ADR-0068 §3).
3. A translator-facing TSV export is available (`gw_cook locale export-tsv`)
   for spreadsheet round-trips; the import step reinvokes the MessageFormat 2
   validator.

Five locales ship at Phase 16 exit: `en-US`, `fr-FR`, `de-DE`, `ja-JP`,
`zh-CN`. Each locale must resolve ≥ 95% of the canonical key set or the
`phase16-locales` CI job fails.

## 4. Content validation

- Every placeholder declared in the English source must appear in every
  translation.
- Ordinal + plural forms use `{count, plural, ...}` — the cook validates
  a locale's plural categories match CLDR 48 for that BCP-47 tag.
- XLIFF notes flagged `critical=yes` block the cook until resolved.

## 5. Null-ICU behaviour

When `GW_ENABLE_ICU=OFF`:

- `NumberFormat::format(int)` = `std::to_string`.
- `NumberFormat::format(double, k)` = `%.*f`.
- `DateTimeFormat::format(ms)` = ISO-8601 `YYYY-MM-DDTHH:MM:SSZ`.
- `plural_category` = English rules (`1 → One`, else `Other`).
- `to_lower/upper_locale` = ASCII-safe `std::tolower/toupper`.

These fall-backs are **documented** in the a11y-selfcheck as "degraded
locale" — they keep `dev-*` deterministic without pretending to be
localised.

## 6. Header quarantine

`<unicode/*.h>` is included only in `impl/icu_backend.cpp`. Public
headers use opaque forward declarations + `std::unique_ptr`.
