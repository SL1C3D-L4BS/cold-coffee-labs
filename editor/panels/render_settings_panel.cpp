// editor/panels/render_settings_panel.cpp
// The big left-column "render cockpit". Hosts tone-map, SSAO, exposure and
// the 64-bucket luminance histogram. All sliders mutate the shared
// RenderSettings struct so the renderer can pick them up per-frame.
#include "render_settings_panel.hpp"
#include "editor/render/render_settings.hpp"

#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace gw::editor {

namespace {

// Seed a plausible histogram shape until the live-luminance reducer lands in
// the renderer (Phase 8). The curve is a gentle log-normal that reacts to
// the editor's "average luminance" so the UI still feels coupled.
void synthesise_histogram(render::LuminanceHistogram& h, float avg) noexcept {
    const float mean   = std::clamp(avg * 32.f, 0.f, 62.f);
    const float stddev = 8.f;
    float total = 0.f;
    for (std::size_t i = 0; i < h.buckets.size(); ++i) {
        const float x = static_cast<float>(i) - mean;
        const float v = std::exp(-0.5f * (x * x) / (stddev * stddev));
        h.buckets[i]  = v;
        total        += v;
    }
    if (total > 0.f) {
        for (auto& v : h.buckets) v /= total;
    }
}

void draw_tonemap(render::ToneMapParams& tm) {
    if (!ImGui::CollapsingHeader("Tone map options",
                                  ImGuiTreeNodeFlags_DefaultOpen)) return;
    ImGui::SliderFloat("Max brightness",  &tm.max_brightness,  0.1f, 4.f,  "%.3f");
    ImGui::SliderFloat("Contrast",        &tm.contrast,        0.5f, 4.f,  "%.3f");
    ImGui::SliderFloat("Linear section",  &tm.linear_section,  0.f,  1.f,  "%.3f");
    ImGui::SliderFloat("Black lightness", &tm.black_lightness, 0.f,  0.5f, "%.3f");
    ImGui::SliderFloat("Pedestal",        &tm.pedestal,        0.f,  0.5f, "%.3f");
    ImGui::SliderFloat("Gamma (tone)",    &tm.gamma,           1.f,  3.f,  "%.3f");
}

void draw_ssao(render::SSAOParams& ssao) {
    if (!ImGui::CollapsingHeader("SSAO",
                                  ImGuiTreeNodeFlags_DefaultOpen)) return;
    ImGui::Checkbox("Enabled##ssao",  &ssao.enabled);
    ImGui::SliderInt  ("Samples",     &ssao.sample_count, 4, 64);
    ImGui::SliderFloat("Radius",      &ssao.radius,       0.01f, 2.f,  "%.3f");
    ImGui::SliderFloat("Bias",        &ssao.bias,         0.f,   0.1f, "%.4f");
    ImGui::SliderFloat("Power",       &ssao.power,        0.5f,  8.f,  "%.3f");
}

void draw_exposure(render::ExposureParams& ex, render::LuminanceHistogram& hist) {
    if (!ImGui::CollapsingHeader("Luminance histogram / exposure",
                                  ImGuiTreeNodeFlags_DefaultOpen)) return;

    synthesise_histogram(hist, ex.average_luminance);
    ImGui::PlotHistogram("##lum_hist", hist.buckets.data(),
                          static_cast<int>(hist.buckets.size()),
                          0, nullptr, FLT_MAX, FLT_MAX, ImVec2{-1.f, 78.f});

    ImGui::SliderFloat("Min log luminance", &ex.min_log_luminance, -8.f, 0.f,  "%.3f");
    ImGui::SliderFloat("Max log luminance", &ex.max_log_luminance,  0.f, 8.f,  "%.3f");
    ImGui::SliderFloat("Gamma (output)",    &ex.gamma,              1.f, 3.f,  "%.3f");
    ImGui::Text("Average luminance : %.6f", ex.average_luminance);
}

}  // namespace

void RenderSettingsPanel::on_imgui_render(EditorContext& /*ctx*/) {
    if (!visible_) return;
    ImGui::Begin(name(), &visible_);

    if (!settings_) {
        ImGui::TextDisabled("No RenderSettings bound");
        ImGui::End();
        return;
    }

    draw_tonemap(settings_->tonemap);
    ImGui::Spacing();
    draw_ssao(settings_->ssao);
    ImGui::Spacing();
    draw_exposure(settings_->exposure, settings_->histogram);

    ImGui::End();
}

}  // namespace gw::editor
