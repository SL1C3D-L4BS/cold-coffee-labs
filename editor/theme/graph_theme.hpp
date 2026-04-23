#pragma once
// ImNodes style from active Theme.graph (VScript, Shader Forge, etc.).

#include "editor/theme/palette.hpp"
#include "engine/vscript/ir.hpp"

#include <cstdint>

namespace gw::editor::theme {

/// Writes `GraphTheme` into the current ImNodes context's `GetStyle().Colors[]`.
void apply_graph_theme_to_current_imnodes(const GraphTheme& g) noexcept;

[[nodiscard]] std::uint32_t imnodes_u32(const Color32& c) noexcept;

/// Per-node kind title/background tints (use around BeginNode/EndNode).
void push_vscript_node_style(gw::vscript::NodeKind k, const GraphTheme& g) noexcept;
void pop_vscript_node_style() noexcept;

} // namespace gw::editor::theme
