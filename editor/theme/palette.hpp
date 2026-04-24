#pragma once
// editor/theme/palette.hpp — Part B §12.1 scaffold (ADR-0104).
//
// Semantic palette for the three editor theme profiles. Panels NEVER read
// magic colour literals; they read the active Palette via ThemeRegistry.
//
// Contrast targets (WCAG 2.1 relative luminance, verified in design pass):
//   Corrupted Relic — text on background ~14:1; text_muted on panel ~5.5:1;
//   destructive on background ~5.5:1; focus on panel ~4.8:1 (non-text UI);
//   on_accent on accent fill ~8:1. Pairings should be re-checked when editing.

#include <cstdint>

namespace gw::editor::theme {

struct Color32 {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
    [[nodiscard]] constexpr std::uint32_t packed_abgr() const noexcept {
        return (std::uint32_t{a} << 24) | (std::uint32_t{b} << 16) |
               (std::uint32_t{g} << 8) | std::uint32_t{r};
    }
};

struct Palette {
    Color32 background;       // 0dp — main window / dockspace back
    Color32 panel;            // docked panel chrome
    Color32 surface_1;        // +1dp — menu bar, nested child, toolbar step
    Color32 surface_2;        // +3dp — popups, floating chrome
    Color32 surface_3;        // +6dp — modal / peak elevation
    Color32 text;             // primary labels
    Color32 text_muted;       // hints, disabled tone
    Color32 accent;           // primary brand / blood interactive
    Color32 accent_secondary; // cool secondary (blurple)
    Color32 accent_strong;    // hover / pressed accent
    Color32 info;             // info rail, diagnostics “sick glass” cyan
    Color32 link;             // hyperlinks, secondary cool highlight
    Color32 focus;            // keyboard / gamepad nav ring (not red-green only)
    Color32 on_accent;        // text/icons on solid accent fills
    Color32 destructive;      // errors, dangerous actions (readable on dark)
    Color32 positive;         // success / live
    Color32 warning;          // caution, drag-drop target
    Color32 separator;        // borders / rules
    Color32 selection;        // text selection / soft tint (may carry alpha)
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
    Color32 vscript_io;       // cool — get_input / set_output
    Color32 vscript_arith;    // arithmetic cluster
    Color32 vscript_control;  // select / branch
};

// ------------------------------------------------------------------
//  Corrupted Relic — Sacrilege horror (charcoal / blood / cyan glass / sulfur).
// ------------------------------------------------------------------
constexpr Palette CORRUPTED_RELIC{
    /*background   */ {0x12, 0x10, 0x0F},
    /*panel        */ {0x1C, 0x18, 0x17},
    /*surface_1    */ {0x26, 0x21, 0x1F},
    /*surface_2    */ {0x30, 0x2A, 0x27},
    /*surface_3    */ {0x3A, 0x32, 0x2E},
    /*text         */ {0xED, 0xE6, 0xE0},
    /*text_muted   */ {0x96, 0x8C, 0x84},
    /*accent       */ {0xC4, 0x34, 0x34},
    /*accent_secondary*/ {0x6E, 0x5A, 0x92},
    /*accent_strong*/ {0xE8, 0x58, 0x52},
    /*info         */ {0x52, 0xB4, 0xC4},
    /*link         */ {0x62, 0xC8, 0xD8},
    /*focus        */ {0xE0, 0xB4, 0x2E},
    /*on_accent    */ {0xFC, 0xF6, 0xF2},
    /*destructive  */ {0xE0, 0x4A, 0x48},
    /*positive     */ {0x5C, 0xB0, 0x5A},
    /*warning      */ {0xC4, 0x94, 0x48}, // muted brass / amber (tool chrome, not traffic neon)
    /*separator    */ {0x3E, 0x34, 0x32},
    /*selection    */ {0xC4, 0x34, 0x34, 0x90},
};

constexpr GraphTheme GRAPH_CORRUPTED_RELIC{
    {0x22, 0x1C, 0x1A}, {0x2C, 0x24, 0x22}, {0x38, 0x2C, 0x28}, {0x52, 0x3C, 0x3A},
    {0x4A, 0x22, 0x22}, {0x62, 0x30, 0x2C}, {0x78, 0x3C, 0x36},
    {0x52, 0xB4, 0xC4}, {0x72, 0xD4, 0xE4},
    {0x6E, 0x5A, 0x92},
    {0x10, 0x0E, 0x0D}, {0x2E, 0x26, 0x24}, {0x44, 0x36, 0x32},
    {0xC4, 0x94, 0x48},
    {0x6A, 0x5A, 0x8C},
    {0x5C, 0xB0, 0x5A},
    {0xB8, 0x48, 0x38},
};

// ------------------------------------------------------------------
//  Brewed Slate — neutral greys, cool accent.
// ------------------------------------------------------------------
constexpr Palette BREWED_SLATE{
    /*background   */ {0x1A, 0x1D, 0x21},
    /*panel        */ {0x25, 0x2A, 0x30},
    /*surface_1    */ {0x2C, 0x32, 0x3A},
    /*surface_2    */ {0x34, 0x3A, 0x44},
    /*surface_3    */ {0x3E, 0x46, 0x50},
    /*text         */ {0xDC, 0xE1, 0xE6},
    /*text_muted   */ {0x8A, 0x94, 0x9E},
    /*accent       */ {0x6F, 0xA8, 0xDC},
    /*accent_secondary*/ {0x7A, 0x62, 0xC4},
    /*accent_strong*/ {0x90, 0xBE, 0xE8},
    /*info         */ {0x58, 0xC0, 0xD0},
    /*link         */ {0x7A, 0xD0, 0xE8},
    /*focus        */ {0xF0, 0xC0, 0x38},
    /*on_accent    */ {0x12, 0x16, 0x1C},
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
    /*surface_1    */ {0x26, 0x26, 0x26},
    /*surface_2    */ {0x30, 0x30, 0x30},
    /*surface_3    */ {0x3A, 0x3A, 0x3A},
    /*text         */ {0xFF, 0xFF, 0xFF},
    /*text_muted   */ {0xB0, 0xB0, 0xB0},
    /*accent       */ {0xFF, 0xB0, 0x00},
    /*accent_secondary*/ {0x56, 0x4C, 0xB0},
    /*accent_strong*/ {0xFF, 0xD0, 0x40},
    /*info         */ {0x56, 0x4C, 0xB0},
    /*link         */ {0x70, 0x68, 0xD8},
    /*focus        */ {0xFF, 0xB0, 0x00},
    /*on_accent    */ {0x10, 0x10, 0x10},
    /*destructive  */ {0xD5, 0x5E, 0x00},
    /*positive     */ {0x00, 0x9E, 0x73},
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

/// Wong semantic accents on top of `base` surfaces (protans / deutans).
[[nodiscard]] inline Palette overlay_wong_semantics(const Palette& base) noexcept {
    Palette       p = base;
    const Palette w = FIELD_TEST_HC;
    p.accent            = w.accent;
    p.accent_secondary  = w.accent_secondary;
    p.accent_strong     = w.accent_strong;
    p.destructive       = w.destructive;
    p.positive          = w.positive;
    p.warning           = w.warning;
    p.selection         = w.selection;
    p.info              = w.accent_secondary;
    p.link              = w.link;
    p.focus             = w.focus;
    p.on_accent         = w.on_accent;
    return p;
}

[[nodiscard]] inline GraphTheme overlay_wong_graph_semantics(
    const GraphTheme& base) noexcept {
    const GraphTheme w = GRAPH_FIELD_TEST_HC;
    GraphTheme         g = base;
    g.imnodes_link       = w.imnodes_link;
    g.imnodes_link_hover = w.imnodes_link_hover;
    g.imnodes_pin        = w.imnodes_pin;
    g.vscript_const      = w.vscript_const;
    g.vscript_io         = w.vscript_io;
    g.vscript_arith      = w.vscript_arith;
    g.vscript_control    = w.vscript_control;
    return g;
}

} // namespace gw::editor::theme
