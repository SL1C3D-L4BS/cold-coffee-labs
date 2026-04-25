// editor/panels/audit/profiler_panel.cpp — editor GPU timing + ECS counts + Tracy hints.

#include "editor/panels/audit/profiler_panel.hpp"

#include "editor/panels/panel.hpp"
#include "engine/core/log.hpp"
#include "engine/ecs/world.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <imgui.h>

namespace gw::editor::panels::audit {
namespace {

constexpr int kSparkLen = 120;

} // namespace

void ProfilerPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    if (!visible()) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    static std::array<float, kSparkLen> spark{};
    static int                          spark_pos = 0;
    spark[static_cast<std::size_t>(spark_pos) % spark.size()] = ctx.framegraph_gpu_ms;
    spark_pos                                                = (spark_pos + 1) % kSparkLen;

    ImGui::Text("Frame graph GPU (editor pass): %.3f ms", static_cast<double>(ctx.framegraph_gpu_ms));
    const std::size_t nent = (ctx.world != nullptr) ? ctx.world->entity_count() : 0U;
    ImGui::Text("ECS live entities: %zu", nent);
    ImGui::PlotLines("##gpu_spark", spark.data(), kSparkLen, 0, nullptr, 0.f, 20.f, ImVec2(0, 64));

    if (ImGui::CollapsingHeader("Tracy live capture", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(
            "If the engine is built with Tracy and GW_TRACY=1, zones appear in the Tracy "
            "viewer. Use the Console panel to record host/port, then start the viewer before "
            "profiled work.");
        static char host[128] = "127.0.0.1";
        static int  port      = 8086;
        ImGui::InputText("Tracy connect host", host, sizeof host);
        ImGui::InputInt("Tracy connect port", &port);
        if (port < 1) {
            port = 1;
        }
        if (port > 65535) {
            port = 65535;
        }
        if (ImGui::Button("Log Tracy target")) {
            char buf[192];
            (void)std::snprintf(
                buf, sizeof buf, "Tracy viewer: %s:%d (requires Tracy-enabled build)", host, port);
            gw::core::log_message(gw::core::LogLevel::Info, "tracy", std::string_view{buf});
        }
    }

    ImGui::End();
}

} // namespace gw::editor::panels::audit
