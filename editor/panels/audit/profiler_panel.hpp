#pragma once
// editor/panels/audit/profiler_panel.hpp — GPU ms sparkline, ECS count, Tracy hints.

#include "editor/panels/panel.hpp"

namespace gw::editor::panels::audit {

class ProfilerPanel final : public gw::editor::IPanel {
public:
    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Profiler"; }
};

} // namespace gw::editor::panels::audit
