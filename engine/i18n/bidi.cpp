// engine/i18n/bidi.cpp — null-ICU BiDi segmentation (ADR-0070).

#include "engine/i18n/bidi.hpp"

#include <cstdint>
#include <utility>

namespace gw::i18n {

namespace {

struct Decoded {
    std::uint32_t codepoint{0};
    std::size_t   bytes{1};
};

Decoded decode_utf8(std::string_view s, std::size_t i) noexcept {
    Decoded d{};
    if (i >= s.size()) return d;
    unsigned char c = static_cast<unsigned char>(s[i]);
    if (c < 0x80) { d.codepoint = c; d.bytes = 1; return d; }
    if ((c >> 5) == 0b110 && i + 1 < s.size()) {
        d.codepoint = ((c & 0x1F) << 6)
                    | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
        d.bytes = 2; return d;
    }
    if ((c >> 4) == 0b1110 && i + 2 < s.size()) {
        d.codepoint = ((c & 0x0F) << 12)
                    | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6)
                    | (static_cast<unsigned char>(s[i + 2]) & 0x3F);
        d.bytes = 3; return d;
    }
    if ((c >> 3) == 0b11110 && i + 3 < s.size()) {
        d.codepoint = ((c & 0x07) << 18)
                    | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12)
                    | ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6)
                    | (static_cast<unsigned char>(s[i + 3]) & 0x3F);
        d.bytes = 4; return d;
    }
    d.codepoint = c;
    d.bytes     = 1;
    return d;
}

gw::ui::TextScript script_for(std::uint32_t cp) noexcept {
    using gw::ui::TextScript;
    // Super-simplified classification sufficient for the Phase-16 corpus.
    if (cp <= 0x007F) return TextScript::Latin;
    if (cp >= 0x0080 && cp <= 0x024F) return TextScript::Latin;
    if (cp >= 0x0370 && cp <= 0x03FF) return TextScript::Greek;
    if (cp >= 0x0400 && cp <= 0x04FF) return TextScript::Cyrillic;
    if (cp >= 0x0590 && cp <= 0x05FF) return TextScript::Hebrew;
    if (cp >= 0x0600 && cp <= 0x06FF) return TextScript::Arabic;
    if (cp >= 0x0700 && cp <= 0x08FF) return TextScript::Arabic;
    if (cp >= 0x3040 && cp <= 0x309F) return TextScript::Hiragana;
    if (cp >= 0x30A0 && cp <= 0x30FF) return TextScript::Katakana;
    if (cp >= 0x3400 && cp <= 0x9FFF) return TextScript::HanS;
    if (cp >= 0xAC00 && cp <= 0xD7AF) return TextScript::Hangul;
    if ((cp >= 0x1F300 && cp <= 0x1F9FF) || (cp >= 0x2600 && cp <= 0x27BF)) return TextScript::Emoji;
    return TextScript::Unknown;
}

bool is_strong_rtl(std::uint32_t cp) noexcept {
    return (cp >= 0x0590 && cp <= 0x05FF)
        || (cp >= 0x0600 && cp <= 0x08FF);
}
bool is_strong_ltr(std::uint32_t cp) noexcept {
    // Roman, Greek, Cyrillic, CJK, Hangul, Hiragana, Katakana all LTR.
    if (cp < 0x80 && (cp < 0x40 || (cp >= 0x5B && cp <= 0x60) || cp >= 0x7B)) {
        // punctuation/controls in ASCII are "neutral" here.
        return false;
    }
    const auto s = script_for(cp);
    using gw::ui::TextScript;
    return s == TextScript::Latin || s == TextScript::Greek || s == TextScript::Cyrillic
        || s == TextScript::HanS   || s == TextScript::HanT || s == TextScript::Hiragana
        || s == TextScript::Katakana || s == TextScript::Hangul;
}

} // namespace

BaseDirection resolve_base_direction(std::string_view utf8, BaseDirection hint) noexcept {
    if (hint != BaseDirection::Auto) return hint;
    for (std::size_t i = 0; i < utf8.size();) {
        const auto d = decode_utf8(utf8, i);
        if (is_strong_rtl(d.codepoint)) return BaseDirection::RTL;
        if (is_strong_ltr(d.codepoint)) return BaseDirection::LTR;
        i += d.bytes;
    }
    return BaseDirection::LTR; // UBA §P2 default
}

std::size_t bidi_segments(std::string_view utf8,
                           BaseDirection    base,
                           std::vector<BidiRun>& out) {
    out.clear();
    if (utf8.empty()) return 0;

    const BaseDirection effective = resolve_base_direction(utf8, base);
    using gw::ui::TextDirection;
    using gw::ui::TextScript;

    TextDirection curr_dir   = (effective == BaseDirection::RTL) ? TextDirection::RTL : TextDirection::LTR;
    TextScript    curr_script = TextScript::Unknown;
    std::uint32_t run_start   = 0;

    auto flush = [&](std::uint32_t end, TextDirection d, TextScript s) {
        if (end <= run_start) return;
        BidiRun r{};
        r.start_byte = run_start;
        r.size_bytes = end - run_start;
        r.direction  = d;
        r.script     = s;
        out.push_back(r);
    };

    for (std::size_t i = 0; i < utf8.size();) {
        const auto d = decode_utf8(utf8, i);
        const auto s = script_for(d.codepoint);
        TextDirection dir = curr_dir;
        if (is_strong_rtl(d.codepoint)) dir = TextDirection::RTL;
        else if (is_strong_ltr(d.codepoint)) dir = TextDirection::LTR;
        // neutrals inherit curr_dir

        // Open a new run if direction or script changes (empty → initialise).
        if (curr_script == TextScript::Unknown) {
            curr_script = s;
            curr_dir    = dir;
        } else if (dir != curr_dir || (s != TextScript::Unknown && s != curr_script)) {
            flush(static_cast<std::uint32_t>(i), curr_dir, curr_script);
            run_start   = static_cast<std::uint32_t>(i);
            curr_script = (s == TextScript::Unknown) ? curr_script : s;
            curr_dir    = dir;
        }
        i += d.bytes;
    }
    flush(static_cast<std::uint32_t>(utf8.size()), curr_dir, curr_script);
    return out.size();
}

} // namespace gw::i18n
