#pragma once
// Machine-readable required panel names (bootstrap + doctest).

#include <array>
#include <string_view>

namespace gw::editor::panels::sacrilege {

/// Window menu / PanelRegistry names that must resolve for a green readiness check.
inline constexpr std::array<std::string_view, 12> kRequiredSacrilegePanelNames = {
    "Act / Phase",
    "AI Director Sandbox",
    "Circle Editor",
    "Dialogue Graph",
    "Editor Copilot",
    "Encounter Editor",
    "Material Forge",
    "PCG Node Graph",
    "Sacrilege Library",
    "Sacrilege Readiness",
    "Shader Forge",
    "Sin-Signature",
};

} // namespace gw::editor::panels::sacrilege
