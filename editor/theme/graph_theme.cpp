// editor/theme/graph_theme.cpp

#include "editor/theme/graph_theme.hpp"

#include <imnodes.h>

namespace gw::editor::theme {
namespace {

Color32 lerp_c(const Color32& a, const Color32& b, float t) noexcept {
    t = (t < 0.f) ? 0.f : (t > 1.f) ? 1.f : t;
    const auto l = [t](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(
            static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t + 0.5f);
    };
    return {l(a.r, b.r), l(a.g, b.g), l(a.b, b.b), l(a.a, b.a)};
}

} // namespace

std::uint32_t imnodes_u32(const Color32& c) noexcept {
    return IM_COL32(c.r, c.g, c.b, c.a);
}

void apply_graph_theme_to_current_imnodes(const GraphTheme& g) noexcept {
    ImNodes::StyleColorsDark();
    ImNodesStyle& s = ImNodes::GetStyle();
    s.Flags |= ImNodesStyleFlags_GridLines;

    s.Colors[ImNodesCol_NodeBackground]         = imnodes_u32(g.imnodes_node_bg);
    s.Colors[ImNodesCol_NodeBackgroundHovered]   = imnodes_u32(g.imnodes_node_bg_hovered);
    s.Colors[ImNodesCol_NodeBackgroundSelected]  = imnodes_u32(g.imnodes_node_bg_sel);
    s.Colors[ImNodesCol_NodeOutline]             = imnodes_u32(g.imnodes_node_outline);
    s.Colors[ImNodesCol_TitleBar]                = imnodes_u32(g.imnodes_title);
    s.Colors[ImNodesCol_TitleBarHovered]         = imnodes_u32(g.imnodes_title_hov);
    s.Colors[ImNodesCol_TitleBarSelected]        = imnodes_u32(g.imnodes_title_sel);
    s.Colors[ImNodesCol_Link]                    = imnodes_u32(g.imnodes_link);
    s.Colors[ImNodesCol_LinkHovered]             = imnodes_u32(g.imnodes_link_hover);
    s.Colors[ImNodesCol_Pin]                     = imnodes_u32(g.imnodes_pin);
    s.Colors[ImNodesCol_PinHovered] = imnodes_u32(lerp_c(g.imnodes_pin, g.imnodes_link_hover, 0.5f));
    s.Colors[ImNodesCol_GridBackground]   = imnodes_u32(g.imnodes_grid_bg);
    s.Colors[ImNodesCol_GridLine]         = imnodes_u32(g.imnodes_grid_line);
    s.Colors[ImNodesCol_GridLinePrimary]  = imnodes_u32(g.imnodes_grid_primary);
    s.Colors[ImNodesCol_BoxSelector] = imnodes_u32(lerp_c(g.imnodes_title, g.imnodes_link, 0.35f));
    s.Colors[ImNodesCol_BoxSelectorOutline] = imnodes_u32(g.imnodes_node_outline);
}

void push_vscript_node_style(const gw::vscript::NodeKind k, const GraphTheme& g) noexcept {
    using gw::vscript::NodeKind;
    Color32 title = g.imnodes_title;
    Color32 bg    = g.imnodes_node_bg;
    switch (k) {
    case NodeKind::Const:
        title = g.vscript_const;
        bg    = lerp_c(g.imnodes_node_bg, g.vscript_const, 0.22f);
        break;
    case NodeKind::GetInput: [[fallthrough]];
    case NodeKind::SetOutput:
        title = g.vscript_io;
        bg    = lerp_c(g.imnodes_node_bg, g.vscript_io, 0.2f);
        break;
    case NodeKind::Add: [[fallthrough]];
    case NodeKind::Sub: [[fallthrough]];
    case NodeKind::Mul:
        title = g.vscript_arith;
        bg    = lerp_c(g.imnodes_node_bg, g.vscript_arith, 0.2f);
        break;
    case NodeKind::Select:
        title = g.vscript_control;
        bg    = lerp_c(g.imnodes_node_bg, g.vscript_control, 0.2f);
        break;
    }
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, imnodes_u32(title));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered,
                            imnodes_u32(lerp_c(title, g.imnodes_link_hover, 0.12f)));
    ImNodes::PushColorStyle(ImNodesCol_NodeBackground, imnodes_u32(bg));
    ImNodes::PushColorStyle(ImNodesCol_NodeOutline, imnodes_u32(lerp_c(g.imnodes_node_outline, title, 0.25f)));
}

void pop_vscript_node_style() noexcept {
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
}

} // namespace gw::editor::theme
