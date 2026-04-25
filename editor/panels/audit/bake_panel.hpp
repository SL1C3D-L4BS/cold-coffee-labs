#pragma once
// editor/panels/audit/bake_panel.hpp — gw_cook integration (Wave 1).

#include "editor/panels/panel.hpp"

#include <atomic>
#include <array>
#include <mutex>
#include <string>
#include <thread>

namespace gw::editor::panels::audit {

class BakePanel final : public gw::editor::IPanel {
public:
    BakePanel() = default;
    ~BakePanel() override;

    void on_imgui_render(gw::editor::EditorContext& ctx) override;
    [[nodiscard]] const char* name() const override { return "Bake"; }

private:
    void append_log(std::string chunk);

    std::mutex              log_mu_;
    std::string             log_;
    std::atomic<bool>       running_{false};
    std::atomic<bool>       cancel_{false};
    std::thread             worker_{};
    std::atomic<int>        last_exit_{-1};
    bool                    force_cook_    = false;
    bool                    verbose_cook_  = true;
    std::array<char, 192>   filter_glob_{};
};

} // namespace gw::editor::panels::audit
