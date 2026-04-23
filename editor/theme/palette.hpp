#pragma once
// editor/theme/palette.hpp — Part B §12.1 scaffold (ADR-0104).
//
// Semantic palette for the three editor theme profiles. Panels NEVER read
// magic colour literals; they read the active Palette via ThemeRegistry.

#include <cstdint>

namespace gw::editor::theme {

struct Color32 {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
    [[nodiscard]] constexpr std::uint32_t packed_abgr() const noexcept {
        return (std::uint32_t{a} << 24) | (std::uint32_t{b} << 16) |
               (std::uint32_t{g} <<  8) |  std::uint32_t{r};
    }
};

struct Palette {
    Color32 background;       // window background
    Color32 panel;            // dockspace panel background
    Color32 text;             // default text
    Color32 text_muted;       // disabled / hint
    Color32 accent;           // primary interactive
    Color32 accent_strong;    // hover / active
    Color32 destructive;      // delete / kill actions
    Color32 positive;         // save / commit / success
    Color32 separator;        // rules between sections
    Color32 selection;        // selection highlight
};

// ------------------------------------------------------------------
//  Corrupted Relic — the full horror aesthetic (opt-in).
//  #111214 Infected Veins   / #2A2522 Burnt Bone / #E0D7C6 Forgotten Scripture
//  #C9A63B Yellow Paint     / #4A1515 Dried Gules / #7A5C2E Blasphemous Gold
// ------------------------------------------------------------------
constexpr Palette CORRUPTED_RELIC{
    /*background   */ {0x11, 0x12, 0x14},
    /*panel        */ {0x2A, 0x25, 0x22},
    /*text         */ {0xE0, 0xD7, 0xC6},
    /*text_muted   */ {0x7A, 0x6E, 0x5C},
    /*accent       */ {0xC9, 0xA6, 0x3B},
    /*accent_strong*/ {0xE8, 0xC4, 0x4E},
    /*destructive  */ {0x4A, 0x15, 0x15},
    /*positive     */ {0x7A, 0x5C, 0x2E},
    /*separator    */ {0x3A, 0x32, 0x2C},
    /*selection    */ {0x6F, 0x4F, 0x22, 0xB0},
};

// ------------------------------------------------------------------
//  Brewed Slate — the a11y default (neutral greys, subtle blue).
// ------------------------------------------------------------------
constexpr Palette BREWED_SLATE{
    /*background   */ {0x1A, 0x1D, 0x21},
    /*panel        */ {0x25, 0x2A, 0x30},
    /*text         */ {0xDC, 0xE1, 0xE6},
    /*text_muted   */ {0x8A, 0x94, 0x9E},
    /*accent       */ {0x6F, 0xA8, 0xDC},
    /*accent_strong*/ {0x90, 0xBE, 0xE8},
    /*destructive  */ {0xC7, 0x54, 0x3E},
    /*positive     */ {0x85, 0xB3, 0x58},
    /*separator    */ {0x35, 0x3B, 0x42},
    /*selection    */ {0x4A, 0x70, 0x95, 0xA0},
};

// ------------------------------------------------------------------
//  Field Test (High Contrast) — WCAG 2.2 AA / Wong palette.
// ------------------------------------------------------------------
constexpr Palette FIELD_TEST_HC{
    /*background   */ {0x0D, 0x0D, 0x0D},
    /*panel        */ {0x1E, 0x1E, 0x1E},
    /*text         */ {0xFF, 0xFF, 0xFF},
    /*text_muted   */ {0xB0, 0xB0, 0xB0},
    /*accent       */ {0xFF, 0xB0, 0x00},   // Wong yellow
    /*accent_strong*/ {0xFF, 0xD0, 0x40},
    /*destructive  */ {0xD5, 0x5E, 0x00},   // Wong vermillion
    /*positive     */ {0x00, 0x9E, 0x73},   // Wong green
    /*separator    */ {0x50, 0x50, 0x50},
    /*selection    */ {0xFF, 0xB0, 0x00, 0x80},
};

} // namespace gw::editor::theme
