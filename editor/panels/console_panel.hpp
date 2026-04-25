#pragma once
// editor/panels/console_panel.hpp — in-memory log (gw::core::log) with filter + pause.

#include "editor/panels/panel.hpp"
#include "engine/core/log.hpp"

#include <array>
#include <vector>

namespace gw::editor {

class ConsolePanel final : public IPanel {
public:
    ConsolePanel() = default;

    void on_imgui_render(EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Console"; }

private:
    std::array<char, 256>       filter_{};
    bool                        paused_      = false;
    bool                        last_pause_  = false;
    std::vector<gw::core::LogLineEntry> snapshot_{};
    int                         tracy_port_ = 8086;
    std::array<char, 128>       tracy_host_{"127.0.0.1"};
};

} // namespace gw::editor
