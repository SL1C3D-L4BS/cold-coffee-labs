// engine/ui/text_shaper.cpp — ADR-0028 Wave 11D.
//
// Null-backend shaper: one glyph per UTF-8 codepoint, advance is a
// constant fraction of pixel_size. This is enough for unit tests, the
// console, and the sandbox HUD where we ship ASCII-only strings. The
// HarfBuzz build replaces the body of shape() in the gated .cpp variant.

#include "engine/ui/text_shaper.hpp"
#include "engine/ui/font_library.hpp"

namespace gw::ui {

namespace {

// Minimal UTF-8 decoder. Returns (codepoint, bytes_consumed).
struct Decoded { std::uint32_t cp; std::uint32_t len; };

Decoded decode_utf8(const char* s, std::size_t available) noexcept {
    if (available == 0) return {0, 0};
    auto u0 = static_cast<unsigned char>(s[0]);
    if (u0 < 0x80) return {u0, 1};
    auto need = [&](std::uint32_t n, std::uint32_t cp_base) -> Decoded {
        if (available < n) return {0xFFFDu, 1};
        std::uint32_t cp = cp_base;
        for (std::uint32_t i = 1; i < n; ++i) {
            auto u = static_cast<unsigned char>(s[i]);
            if ((u & 0xC0) != 0x80) return {0xFFFDu, i};
            cp = (cp << 6) | (u & 0x3F);
        }
        return {cp, n};
    };
    if ((u0 & 0xE0) == 0xC0) return need(2, u0 & 0x1F);
    if ((u0 & 0xF0) == 0xE0) return need(3, u0 & 0x0F);
    if ((u0 & 0xF8) == 0xF0) return need(4, u0 & 0x07);
    return {0xFFFDu, 1};
}

TextScript script_for_codepoint(std::uint32_t cp) noexcept {
    // Fast path: ASCII / Latin-1.
    if (cp <= 0x024F) return TextScript::Latin;
    if (cp >= 0x0400 && cp <= 0x04FF) return TextScript::Cyrillic;
    if (cp >= 0x0370 && cp <= 0x03FF) return TextScript::Greek;
    if (cp >= 0x0600 && cp <= 0x06FF) return TextScript::Arabic;
    if (cp >= 0x0590 && cp <= 0x05FF) return TextScript::Hebrew;
    if (cp >= 0x3040 && cp <= 0x309F) return TextScript::Hiragana;
    if (cp >= 0x30A0 && cp <= 0x30FF) return TextScript::Katakana;
    if (cp >= 0xAC00 && cp <= 0xD7AF) return TextScript::Hangul;
    if (cp >= 0x4E00 && cp <= 0x9FFF) return TextScript::HanS;
    // Emoji blocks: Misc Symbols/Pictographs, Transport, Dingbats.
    if ((cp >= 0x1F300 && cp <= 0x1FAFF) ||
        (cp >= 0x2600  && cp <= 0x27BF))  return TextScript::Emoji;
    return TextScript::Unknown;
}

} // namespace

TextScript TextShaper::detect_script(std::string_view utf8) noexcept {
    for (std::size_t i = 0; i < utf8.size();) {
        auto d = decode_utf8(utf8.data() + i, utf8.size() - i);
        if (d.len == 0) break;
        auto s = script_for_codepoint(d.cp);
        if (s != TextScript::Unknown) return s;
        i += d.len;
    }
    return TextScript::Latin;
}

std::size_t TextShaper::shape(const FontLibrary& fonts, const ShapeInput& in,
                              std::vector<ShapedGlyph>& out) const {
    out.clear();
    const auto* face = fonts.face(in.face);
    if (!face) return 0;

    // Advance heuristic: 0.6 * pixel_size for proportional text, 1.0 for CJK.
    const std::int32_t adv_lat =
        static_cast<std::int32_t>(in.pixel_size * 64) * 60 / 100;   // 0.60em in 26.6
    const std::int32_t adv_cjk =
        static_cast<std::int32_t>(in.pixel_size * 64);              // 1.00em
    const std::int32_t adv_emoji = adv_cjk;

    for (std::size_t i = 0; i < in.utf8.size();) {
        auto d = decode_utf8(in.utf8.data() + i, in.utf8.size() - i);
        if (d.len == 0) break;
        ShapedGlyph g{};
        g.face = in.face;
        g.glyph_index = d.cp;  // null backend: cp == glyph index
        g.cluster = static_cast<std::uint32_t>(i);
        auto s = script_for_codepoint(d.cp);
        switch (s) {
            case TextScript::HanS: case TextScript::HanT:
            case TextScript::Hiragana: case TextScript::Katakana:
            case TextScript::Hangul:
                g.x_advance_26_6 = adv_cjk; break;
            case TextScript::Emoji:
                g.x_advance_26_6 = adv_emoji;
                g.color = face->has_emoji_color;
                break;
            default:
                g.x_advance_26_6 = adv_lat;
        }
        out.push_back(g);
        i += d.len;
    }

    if (in.direction == TextDirection::RTL) {
        // Simple mirror: reverse the glyph order; byte-cluster info preserved.
        for (std::size_t lo = 0, hi = out.size(); lo + 1 < hi; ++lo, --hi) {
            std::swap(out[lo], out[hi - 1]);
        }
    }
    return out.size();
}

} // namespace gw::ui
