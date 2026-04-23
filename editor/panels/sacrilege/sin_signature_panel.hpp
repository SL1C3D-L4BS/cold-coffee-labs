#pragma once
// editor/panels/sacrilege/sin_signature_panel.hpp — Phase 22 scaffold.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::sacrilege {

class SinSignaturePanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Sin-Signature"; }
};

} // namespace gw::editor::panels::sacrilege
