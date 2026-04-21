// editor/panels/stats_panel.cpp
// Implementation of the "Scene" cockpit card: live FPS + frame-time plot
// plus the debug-draw toggle grid the renderer queries each frame.
#include "stats_panel.hpp"
#include "editor/render/render_settings.hpp"
#include "engine/ecs/world.hpp"

#include <imgui.h>
#include <cstdio>

namespace gw::editor {

namespace {

// Pull the freshest sample out of the ring buffer as the "right-most" tip of
// the mini-plot. ImGui::PlotLines reads values left-to-right so we shuffle
// the ring to start at (head) and wrap.
void plot_history_ring(const char* label,
                       const std::array<float, render::FrameStats::kHistoryCount>& data,
                       std::size_t head,
                       float scale_min, float scale_max,
                       ImVec2 size, const char* overlay = nullptr) {
    std::array<float, render::FrameStats::kHistoryCount> shuffled{};
    const std::size_t N = data.size();
    for (std::size_t i = 0; i < N; ++i) {
        shuffled[i] = data[(head + i) % N];
    }
    ImGui::PlotLines(label, shuffled.data(), static_cast<int>(N), 0,
                     overlay, scale_min, scale_max, size);
}

}  // namespace

void StatsPanel::on_imgui_render(EditorContext& ctx) {
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);

    if (settings_) {
        const auto& s = settings_->stats;

        // Sample the current frame into the ring buffer. We mutate via the
        // owning pointer; the EditorApplication's dt_s is authoritative.
        if (ctx.delta_time_s > 0.f) {
            const float fps    = 1.f / ctx.delta_time_s;
            const float cpu_ms = ctx.delta_time_s * 1000.f;
            settings_->stats.push_sample(fps, cpu_ms);
        }

        ImGui::TextColored(ImVec4{0.46f, 0.79f, 0.65f, 1.f},
                           "FPS  %6.1f", s.fps);
        ImGui::SameLine();
        ImGui::TextDisabled(" |  %.2f ms", s.cpu_ms);

        const ImVec2 plot_size{-1.f, 48.f};
        char overlay[32]; std::snprintf(overlay, sizeof(overlay),
                                         "%.2f ms", s.cpu_ms);
        plot_history_ring("##msplot", s.ms_history, s.history_head,
                           0.f, 33.f, plot_size, overlay);

        ImGui::Separator();
        ImGui::TextDisabled("Entity counts");
        if (ctx.world) {
            std::size_t total = 0;
            ctx.world->visit_entities(
                [&](gw::ecs::Entity) noexcept { ++total; });
            ImGui::Text("entities : %zu", total);
        } else {
            ImGui::TextDisabled("(no world bound)");
        }

        ImGui::Separator();
        ImGui::TextDisabled("Debug draw");

        auto& d = settings_->debug;
        ImGui::Checkbox("Grid",          &d.draw_grid);          ImGui::SameLine();
        ImGui::Checkbox("Gizmos",        &d.draw_gizmos);
        ImGui::Checkbox("Wireframe",     &d.draw_wireframe);     ImGui::SameLine();
        ImGui::Checkbox("Debug lines",   &d.draw_debug_lines);
        ImGui::Checkbox("Bounds",        &d.draw_bounds);        ImGui::SameLine();
        ImGui::Checkbox("Light volumes", &d.draw_light_volumes);
        ImGui::Checkbox("Freeze culling", &d.freeze_culling);
    } else {
        ImGui::TextDisabled("No RenderSettings bound");
    }

    ImGui::End();
}

}  // namespace gw::editor
