#pragma once
// editor/panels/sacrilege/dialogue_graph_panel.hpp — Phase 22 W148.

#include "editor/panels/panel.hpp"
#include "engine/narrative/narrative_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gw::editor::panels::sacrilege {

/// Editor-side authoring row. Compiles to a runtime `DialogueLine` when the
/// panel writes a `.gwdlg` blob.
struct DialogueAuthorRow {
    std::uint32_t   line_id = 0;
    gw::narrative::Speaker speaker = gw::narrative::Speaker::Malakor;
    gw::narrative::Act     act_gate = gw::narrative::Act::I;
    std::uint32_t   circle_gate_mask = 0;
    std::uint8_t    cruelty_min = 0,   cruelty_max = 255;
    std::uint8_t    precision_min = 0, precision_max = 255;
    std::string     text;
};

/// Editable dialogue graph — the runtime `DialogueGraph` is a read-only span
/// over cooked data; this is the *authoring* side.
class DialogueGraphPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Dialogue Graph"; }

    // Data-model API (tested without ImGui).
    [[nodiscard]] std::size_t row_count() const noexcept { return rows_.size(); }
    [[nodiscard]] const DialogueAuthorRow& row(std::size_t i) const noexcept { return rows_[i]; }
    DialogueAuthorRow&       mutable_row(std::size_t i)       noexcept { return rows_[i]; }
    void                     append_row(const DialogueAuthorRow& r) { rows_.push_back(r); }
    void                     clear() { rows_.clear(); }

private:
    std::vector<DialogueAuthorRow> rows_;
};

} // namespace gw::editor::panels::sacrilege
