// editor/panels/audit/heatmap_panel.cpp — Wave 1: director encounter pressure preview.

#include "editor/panels/audit/heatmap_panel.hpp"

#include "editor/config/editor_paths.hpp"
#include "engine/services/director/director_service.hpp"
#include "engine/services/director/schema/director.hpp"

#include <imgui.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace gw::editor::panels::audit {
namespace {

[[nodiscard]] const char* intensity_label(gw::services::director::IntensityState s) noexcept {
    using IS = gw::services::director::IntensityState;
    switch (s) {
    case IS::Relax: return "Relax";
    case IS::BuildUp: return "BuildUp";
    case IS::SustainedPeak: return "SustainedPeak";
    case IS::PeakFade: return "PeakFade";
    }
    return "?";
}

[[nodiscard]] std::filesystem::path heatmap_preset_path() {
    return gw::editor::config::editor_data_root() / "audit_heatmap_preset.ini";
}

[[nodiscard]] std::string trim(std::string_view sv) {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r')) {
        sv.remove_suffix(1);
    }
    return std::string{sv};
}

bool load_heatmap_preset(float& sin_out, std::uint64_t& seed_out, std::uint64_t& tick_out,
                         int& state_out) {
    const auto p = heatmap_preset_path();
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        return false;
    }
    bool got_sin = false, got_seed = false, got_tick = false, got_st = false;
    std::string    line;
    while (std::getline(f, line)) {
        const std::string_view sv{line};
        if (sv.empty() || sv[0] == '#' || sv[0] == ';') {
            continue;
        }
        const auto eq = sv.find('=');
        if (eq == std::string_view::npos) {
            continue;
        }
        const std::string k = trim(sv.substr(0, eq));
        const std::string v = trim(sv.substr(eq + 1));
        if (k == "normalized_sin") {
            sin_out  = static_cast<float>(std::atof(v.c_str()));
            got_sin  = true;
        } else if (k == "seed") {
            seed_out = static_cast<std::uint64_t>(std::strtoull(v.c_str(), nullptr, 0));
            got_seed = true;
        } else if (k == "logical_tick") {
            tick_out = static_cast<std::uint64_t>(std::strtoull(v.c_str(), nullptr, 10));
            got_tick = true;
        } else if (k == "intensity_state") {
            state_out = static_cast<int>(std::strtol(v.c_str(), nullptr, 10));
            got_st    = true;
        }
    }
    return got_sin && got_seed && got_tick && got_st;
}

bool save_heatmap_preset(float sin_v, std::uint64_t seed, std::uint64_t tick, int state_i) {
    const auto p = heatmap_preset_path();
    std::ofstream f(p, std::ios::binary);
    if (!f) {
        return false;
    }
    f << "# Greywater editor — Heatmap (director policy preview) preset\n";
    f << "normalized_sin=" << sin_v << "\n";
    f << "seed=" << seed << "\n";
    f << "logical_tick=" << tick << "\n";
    f << "intensity_state=" << state_i << "\n";
    return true;
}

} // namespace

void HeatmapPanel::on_imgui_render(gw::editor::EditorContext& ctx) {
    (void)ctx;
    if (!visible_) {
        return;
    }
    ImGui::Begin(name(), &visible_);

    ImGui::TextUnformatted(
        "Encounter pressure to director policy. Values feed a live "
        "`gw::services::director::evaluate` call; results and spawn scalars show below.");

    static float    normalized_sin_driver = 0.5f;
    static std::uint64_t seed_val         = 0xE001u;
    static std::uint64_t logical_tick_val = 1;
    static int        state_i = 1; // BuildUp
    static gw::services::director::DirectorResult last{};
    static bool       have_last   = false;

    ImGui::SliderFloat("Normalized sin driver", &normalized_sin_driver, 0.f, 1.f);
    ImGui::InputScalar("Seed##hm_seed", ImGuiDataType_U64, &seed_val);
    ImGui::InputScalar("Logical tick##hm_tick", ImGuiDataType_U64, &logical_tick_val);
    const char* st_items[] = {"Relax", "BuildUp", "SustainedPeak", "PeakFade"};
    ImGui::Combo("Intensity state", &state_i, st_items, 4);
    if (state_i < 0) {
        state_i = 0;
    }
    if (state_i > 3) {
        state_i = 3;
    }

    if (ImGui::Button("Evaluate director policy##hm_eval")) {
        gw::services::director::DirectorRequest dr{};
        dr.seed            = seed_val;
        dr.logical_tick    = logical_tick_val;
        dr.normalized_sin  = normalized_sin_driver;
        dr.current_state   = static_cast<gw::services::director::IntensityState>(state_i);
        last               = gw::services::director::evaluate(dr);
        have_last          = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save preset##hm_save")) {
        (void)save_heatmap_preset(normalized_sin_driver, seed_val, logical_tick_val, state_i);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load preset##hm_load")) {
        float         s = normalized_sin_driver;
        std::uint64_t sd = seed_val, tk = logical_tick_val;
        int           st = state_i;
        if (load_heatmap_preset(s, sd, tk, st)) {
            normalized_sin_driver = s;
            seed_val              = sd;
            logical_tick_val      = tk;
            state_i               = st;
            if (state_i < 0) {
                state_i = 0;
            }
            if (state_i > 3) {
                state_i = 3;
            }
        }
    }
    ImGui::TextDisabled("Presets: %s", heatmap_preset_path().string().c_str());

    if (have_last) {
        ImGui::Separator();
        ImGui::TextUnformatted("Last evaluate result");
        ImGui::Text("Next state: %s", intensity_label(last.next_state));
        ImGui::Text("Spawn rate scale: %.3f", static_cast<double>(last.spawn_rate_scale));
        ImGui::Text("Item drop scale: %.3f", static_cast<double>(last.item_drop_scale));
        ImGui::Text("Intensity target: %.3f", static_cast<double>(last.intensity_target));
    } else {
        ImGui::Separator();
        ImGui::TextDisabled("No evaluation yet — press Evaluate.");
    }

    ImGui::End();
}

} // namespace gw::editor::panels::audit
