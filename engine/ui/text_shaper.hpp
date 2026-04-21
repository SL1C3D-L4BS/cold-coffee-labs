#pragma once
// engine/ui/text_shaper.hpp — HarfBuzz-driven shaping pipeline (ADR-0028).
//
// The null build returns per-codepoint 1:1 mappings: each UTF-8 scalar
// becomes one glyph at advance_x = pixel_size * 0.6. This is precisely
// the contract the GlyphAtlas expects, and lets every dependent system
// compile without HarfBuzz.
//
// The GW_UI_HARFBUZZ=1 build replaces `shape()` with a hb_buffer-backed
// implementation; the public API does not change.

#include "engine/ui/ui_types.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace gw::ui {

class FontLibrary;

enum class TextScript : std::uint8_t {
    Unknown = 0,
    Latin,
    Cyrillic,
    Greek,
    Arabic,    // RTL
    Hebrew,    // RTL
    HanS,      // Simplified CJK
    HanT,      // Traditional CJK
    Hiragana,
    Katakana,
    Hangul,
    Emoji,
};

enum class TextDirection : std::uint8_t { LTR = 0, RTL = 1 };

struct ShapedGlyph {
    FontFaceId    face{};
    std::uint32_t glyph_index{0};
    std::int32_t  x_advance_26_6{0};
    std::int32_t  y_advance_26_6{0};
    std::int32_t  x_offset_26_6{0};
    std::int32_t  y_offset_26_6{0};
    std::uint32_t cluster{0};       // byte offset into source string
    bool          color{false};     // COLRv1 emoji glyph
};

struct ShapeInput {
    std::string_view utf8{};
    FontFaceId       face{};
    std::uint32_t    pixel_size{16};
    TextScript       script{TextScript::Latin};
    TextDirection    direction{TextDirection::LTR};
    std::string_view locale{"en"};
};

class TextShaper {
public:
    TextShaper() = default;
    ~TextShaper() = default;
    TextShaper(const TextShaper&) = delete;
    TextShaper& operator=(const TextShaper&) = delete;

    // Shape a UTF-8 run into `out`. Returns the number of glyphs produced.
    // Zero-alloc once `out` has sufficient capacity (reserve a few times
    // the expected glyph count upfront).
    std::size_t shape(const FontLibrary& fonts, const ShapeInput& in,
                      std::vector<ShapedGlyph>& out) const;

    // Detect script from the first few scalars of a UTF-8 string.
    [[nodiscard]] static TextScript detect_script(std::string_view utf8) noexcept;

    // Implied direction from script.
    [[nodiscard]] static TextDirection direction_for(TextScript s) noexcept {
        return (s == TextScript::Arabic || s == TextScript::Hebrew)
            ? TextDirection::RTL
            : TextDirection::LTR;
    }
};

} // namespace gw::ui
