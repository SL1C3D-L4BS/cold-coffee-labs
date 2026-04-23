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
    Color32 positive;         // save / commit / success / “live” indicators
    Color32 warning;          // plots, drag-drop, caution emphasis
    Color32 separator;        // rules between sections
    Color32 selection;        // selection highlight
};

// ImNodes + VScript: chrome and per–node-kind tints (ADR-0104 extension).
struct GraphTheme {
    Color32 imnodes_node_bg;
    Color32 imnodes_node_bg_hovered;
    Color32 imnodes_node_bg_sel;
    Color32 imnodes_node_outline;
    Color32 imnodes_title;
    Color32 imnodes_title_hov;
    Color32 imnodes_title_sel;
    Color32 imnodes_link;
    Color32 imnodes_link_hover;
    Color32 imnodes_pin;
    Color32 imnodes_grid_bg;
    Color32 imnodes_grid_line;
    Color32 imnodes_grid_primary;
    Color32 vscript_const;    // gold / literal
    Color32 vscript_io;       // cool blurple — get_input / set_output
    Color32 vscript_arith;    // arithmetic cluster
    Color32 vscript_control;  // select / branch
};

// ------------------------------------------------------------------
//  Corrupted Relic — Sacrilege horror default (black / blood / sulfur / toxic).
// ------------------------------------------------------------------
constexpr Palette CORRUPTED_RELIC{
    /*background   */ {0x0A, 0x08, 0x08},
    /*panel        */ {0x1A, 0x14, 0x14},
    /*text         */ {0xE8, 0xDC, 0xD0},
    /*text_muted   */ {0x78, 0x6C, 0x62},
    /*accent       */ {0xC4, 0x2A, 0x2A},   // blood red
    /*accent_strong*/ {0xF0, 0x50, 0x50},   // ember
    /*destructive  */ {0x6E, 0x0A, 0x0A},   // dried gules
    /*positive     */ {0x4F, 0x9D, 0x4A},   // sick green
    /*warning      */ {0xD4, 0xA0, 0x17},   // sulfur yellow
    /*separator    */ {0x3A, 0x28, 0x28},
    /*selection    */ {0xC4, 0x2A, 0x2A, 0x90},
};

constexpr GraphTheme GRAPH_CORRUPTED_RELIC{
    {0x1E, 0x18, 0x18}, {0x28, 0x20, 0x20}, {0x32, 0x26, 0x24}, {0x5A, 0x3A, 0x3A},
    {0x4A, 0x1E, 0x1E}, {0x6A, 0x2C, 0x2C}, {0x80, 0x38, 0x38},
    {0x6E, 0x3C, 0x5C}, {0x9A, 0x50, 0x70},
    {0xC8, 0x70, 0x90},
    {0x0E, 0x0A, 0x0A}, {0x2E, 0x22, 0x22}, {0x44, 0x32, 0x32},
    {0xD4, 0xA0, 0x17},
    {0x6A, 0x5A, 0x8C},
    {0x4F, 0x9D, 0x4A},
    {0xB8, 0x40, 0x30},
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
    /*warning      */ {0xF5, 0xA6, 0x23},
    /*separator    */ {0x35, 0x3B, 0x42},
    /*selection    */ {0x4A, 0x70, 0x95, 0xA0},
};

constexpr GraphTheme GRAPH_BREWED_SLATE{
    {0x22, 0x26, 0x2C}, {0x2C, 0x32, 0x3A}, {0x38, 0x3E, 0x48}, {0x4A, 0x55, 0x62},
    {0x32, 0x3A, 0x44}, {0x40, 0x4A, 0x56}, {0x4E, 0x5A, 0x68},
    {0x4A, 0x78, 0xA8}, {0x70, 0xA0, 0xD0},
    {0x6F, 0xA8, 0xDC},
    {0x1A, 0x1D, 0x21}, {0x35, 0x3B, 0x45}, {0x50, 0x58, 0x64},
    {0xD4, 0xB0, 0x20},
    {0x5C, 0x6A, 0xC0},
    {0x60, 0xB0, 0x5A},
    {0x8B, 0x6A, 0xC0},
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
    /*warning      */ {0xFF, 0xB0, 0x00},
    /*separator    */ {0x50, 0x50, 0x50},
    /*selection    */ {0xFF, 0xB0, 0x00, 0x80},
};

constexpr GraphTheme GRAPH_FIELD_TEST_HC{
    {0x1A, 0x1A, 0x1A}, {0x28, 0x28, 0x28}, {0x38, 0x38, 0x38}, {0x80, 0x80, 0x80},
    {0x2A, 0x2A, 0x2A}, {0x3C, 0x3C, 0x3C}, {0x50, 0x50, 0x50},
    {0xF0, 0xA0, 0x00}, {0xFF, 0xD0, 0x40},
    {0xFF, 0xB0, 0x00},
    {0x0D, 0x0D, 0x0D}, {0x50, 0x50, 0x50}, {0x80, 0x80, 0x80},
    {0xFF, 0xB0, 0x00},
    {0x56, 0x4C, 0xB0},
    {0x00, 0x9E, 0x73},
    {0xD5, 0x5E, 0x00},
};

} // namespace gw::editor::theme
