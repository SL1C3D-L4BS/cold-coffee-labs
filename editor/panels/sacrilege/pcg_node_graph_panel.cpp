// editor/panels/sacrilege/pcg_node_graph_panel.cpp — Wave 1: live ImNodes PCG canvas.

#include "editor/panels/sacrilege/pcg_node_graph_panel.hpp"

#include "editor/theme/graph_theme.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>
#include <imnodes.h>

namespace gw::editor::panels::sacrilege {

PcgNodeGraphPanel::PcgNodeGraphPanel() {
    imnodes_ctx_ = ImNodes::CreateContext();
}

PcgNodeGraphPanel::~PcgNodeGraphPanel() {
    if (imnodes_ctx_ != nullptr) {
        ImNodes::DestroyContext(static_cast<ImNodesContext*>(imnodes_ctx_));
        imnodes_ctx_ = nullptr;
    }
}

void PcgNodeGraphPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    (void)ctx;
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::TextUnformatted(
        "Typed procedural graph for Nine-Circles SDR authoring. Nodes compile to "
        "deterministic parameter bundles consumed by Level Architect at cook time.");
    ImGui::Separator();

    ImNodes::SetCurrentContext(static_cast<ImNodesContext*>(imnodes_ctx_));
    gw::editor::theme::apply_graph_theme_to_current_imnodes(
        gw::editor::theme::ThemeRegistry::instance().active().graph);

    ImNodes::BeginNodeEditor();

    constexpr int kNodeNoise   = 1;
    constexpr int kNodeWarp  = 2;
    constexpr int kPinNoiseH = 10;
    constexpr int kPinWarpIn = 20;
    constexpr int kPinWarpOut = 21;
    constexpr int kLinkMain  = 100;

    ImNodes::BeginNode(kNodeNoise);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("sdr_noise");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginOutputAttribute(kPinNoiseH);
    ImGui::TextUnformatted("height_field : f32");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::BeginNode(kNodeWarp);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("domain_warp");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginInputAttribute(kPinWarpIn);
    ImGui::TextUnformatted("field_in");
    ImNodes::EndInputAttribute();
    ImNodes::BeginOutputAttribute(kPinWarpOut);
    ImGui::TextUnformatted("field_out");
    ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::Link(kLinkMain, kPinNoiseH, kPinWarpIn);

    ImNodes::EndNodeEditor();

    ImGui::Separator();
    ImGui::TextDisabled("Serialize / cook hooks share the gw_cook graph once the PCG IR lands.");

    ImGui::End();
}

} // namespace gw::editor::panels::sacrilege
