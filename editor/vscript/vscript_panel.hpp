#pragma once
// editor/vscript/vscript_panel.hpp
// Phase 8 week 047 — Visual Scripting node-graph panel.
//
// This panel is a **projection** of the vscript IR (engine/vscript/ir.hpp),
// not a second ground-truth. CLAUDE.md non-negotiable #14 is explicit: the
// text IR *is* the source of truth. Everything the user sees here is rebuilt
// from the IR every time a graph is loaded; everything the user does here is
// serialised back through `serialize()` before it touches disk.
//
// The widget stack:
//   * ImNodes renders the DAG (nodes, pins, wires).
//   * Node positions are a pure view concern → stored in a sidecar
//     `<script>.layout.json` next to the `.gwvs` text. The IR never carries
//     layout.
//   * A small toolbar lets the user parse / serialize the currently open
//     script, run it through the reference interpreter, and see the returned
//     output map. This is the "Wave 5 interpreter preview" from the plan.
//
// This panel is intentionally a view: no editing operations (add node, wire
// pin, …) are exposed yet — Phase 9 adds the CommandStack-backed editing
// model. For Phase 8 the viewer is sufficient to validate IR round-trip and
// prove the ImNodes integration works.

#include "editor/panels/panel.hpp"

#include "engine/vscript/interpreter.hpp"
#include "engine/vscript/ir.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace gw::editor {

class VScriptPanel final : public IPanel {
public:
    VScriptPanel();
    ~VScriptPanel() override;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "VScript"; }

    // Programmatic entry points (used by tests and by `File → Open…`).
    // Returns true iff parsing + validation succeed.
    bool load_text(std::string_view text);
    bool load_file(const std::string& path);
    bool save_file(const std::string& path) const;

    // Read-only access to the current parsed graph (for tests).
    [[nodiscard]] const gw::vscript::Graph* graph() const noexcept {
        return graph_.has_value() ? &*graph_ : nullptr;
    }

private:
    // --- Rendering -----------------------------------------------------
    void draw_toolbar();
    void draw_graph_canvas();
    void draw_preview_panel();

    // --- Layout sidecar -----------------------------------------------
    // JSON map of `node_id -> {x, y}`. Tiny handwritten reader/writer so
    // we don't pull nlohmann::json into the editor. The format is stable
    // and round-trips: `{"1":[12.0,34.0],"2":[56.0,78.0]}`.
    void apply_layout();               // push positions into ImNodes
    void harvest_layout();              // pull positions out of ImNodes
    void load_layout(const std::string& path);
    void save_layout(const std::string& path) const;

    // --- State ---------------------------------------------------------
    // Current script text — the editable buffer the user types in. When
    // the user clicks "Parse" we feed this into `parse()` and update
    // `graph_` / `parse_error_`.
    std::string text_buf_;
    std::string current_path_;   // disk path, empty = unsaved

    std::optional<gw::vscript::Graph> graph_;
    std::string                       parse_error_;   // from parse() or validate()
    std::string                       exec_error_;    // from execute()

    // node_id → (x,y) in graph-space. Mirrors the ImNodes positions; we
    // sync bidirectionally (on load, on save, on user drag).
    std::unordered_map<std::uint32_t, std::pair<float, float>> layout_;

    // Cached interpreter output from the most recent "Run" button press.
    std::optional<gw::vscript::ExecResult> last_result_;

    // ImNodes context — owned per-panel so multiple vscript panels can
    // coexist without fighting over globals.
    void* imnodes_ctx_ = nullptr;     // struct ImNodesContext*

    // Monotonic "session" id appended to every ImNodes id so we can
    // safely keep the panel alive across reloads without stale ids.
    int generation_ = 0;
};

}  // namespace gw::editor
