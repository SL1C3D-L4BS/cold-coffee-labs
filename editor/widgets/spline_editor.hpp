#pragma once
// editor/widgets/spline_editor.hpp — Part B §12.3 scaffold.

#include <cstdint>
#include <vector>

namespace gw::editor::widgets {

enum class SplineType : std::uint8_t { CatmullRom = 0, Bezier = 1 };

struct SplineKnot {
    float p[3]{0.f, 0.f, 0.f};
    float t[3]{0.f, 0.f, 0.f};  // tangent (Bezier)
};

struct SplineEditorState {
    SplineType type = SplineType::CatmullRom;
    std::vector<SplineKnot> knots;
    int    selected = -1;
    bool   dirty    = false;
};

/// Pure ImGui spline editor — Catmull-Rom / Bezier. Single `BeginChild` +
/// `InvisibleButton` hit areas + custom `ImDrawList` polylines.
/// Returns true if the state mutated this frame.
bool spline_editor(const char* id, SplineEditorState& state) noexcept;

} // namespace gw::editor::widgets
