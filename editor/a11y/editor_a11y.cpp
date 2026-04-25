// editor/a11y/editor_a11y.cpp — Wave 1: persist EditorA11yConfig to editor data root.

#include "editor/a11y/editor_a11y.hpp"

#include "editor/config/editor_paths.hpp"
#include "editor/theme/theme_registry.hpp"

#include <imgui.h>

#include <cctype>
#include <fstream>
#include <string>
#include <string_view>

namespace gw::editor::a11y {
namespace {

[[nodiscard]] std::filesystem::path a11y_file_path() noexcept {
    return gw::editor::config::editor_data_root() / "editor_a11y.toml";
}

[[nodiscard]] std::string trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return std::string{sv};
}

[[nodiscard]] bool parse_bool(std::string_view v) noexcept {
    const std::string t = trim(v);
    if (t == "1" || t == "true" || t == "True" || t == "TRUE" || t == "yes" || t == "Yes" ||
        t == "on" || t == "On" || t == "ON") {
        return true;
    }
    if (t == "0" || t == "false" || t == "False" || t == "FALSE" || t == "no" || t == "No" ||
        t == "off" || t == "Off" || t == "OFF") {
        return false;
    }
    return false;
}

void set_field(std::string_view key, std::string_view val, EditorA11yConfig& c) noexcept {
    if (key == "reduce_corruption") {
        c.reduce_corruption = parse_bool(val);
    } else if (key == "disable_vignette") {
        c.disable_vignette = parse_bool(val);
    } else if (key == "force_high_contrast") {
        c.force_high_contrast = parse_bool(val);
    } else if (key == "force_mono_font") {
        c.force_mono_font = parse_bool(val);
    } else if (key == "keyboard_only_nav") {
        c.keyboard_only_nav = parse_bool(val);
    } else if (key == "colour_blind_wong") {
        c.colour_blind_wong = parse_bool(val);
    } else if (key == "reduce_motion") {
        c.reduce_motion = parse_bool(val);
    }
}

} // namespace

EditorA11yConfig load_from_path(const std::filesystem::path& p) noexcept {
    EditorA11yConfig out{};
    std::ifstream    f(p, std::ios::binary);
    if (!f) {
        return out;
    }
    // Wave-1 files had no table header — accept key=value anywhere until the
    // first `[` line appears; after that only lines under `[editor_a11y]` apply.
    bool        legacy_flat_file = true;
    bool        in_editor_a11y   = false;
    std::string line;
    while (std::getline(f, line)) {
        const std::string_view sv{line};
        if (sv.empty() || sv[0] == '#' || sv[0] == ';') {
            continue;
        }
        const std::string sec = trim(sv);
        if (!sec.empty() && sec.front() == '[' && sec.back() == ']') {
            legacy_flat_file = false;
            const std::string inner = trim(std::string_view{sec}.substr(1, sec.size() - 2));
            in_editor_a11y   = (inner == "editor_a11y");
            continue;
        }
        const auto eq = sv.find('=');
        if (eq == std::string_view::npos) {
            continue;
        }
        if (!legacy_flat_file && !in_editor_a11y) {
            continue;
        }
        const std::string k = trim(sv.substr(0, eq));
        const std::string v = trim(sv.substr(eq + 1));
        set_field(k, v, out);
    }
    return out;
}

EditorA11yConfig load_from_config() noexcept {
    return load_from_path(a11y_file_path());
}

void save_to_path(const EditorA11yConfig& cfg, const std::filesystem::path& p) noexcept {
    std::error_code ec;
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) {
        return;
    }
    f << "# Greywater editor accessibility. Optional section header for strict TOML parsers.\n";
    f << "[editor_a11y]\n";
    f << "# Booleans: true/false, 1/0, yes/no, on/off.\n";
    f << "reduce_corruption = " << (cfg.reduce_corruption ? "true" : "false") << '\n';
    f << "disable_vignette = " << (cfg.disable_vignette ? "true" : "false") << '\n';
    f << "force_high_contrast = " << (cfg.force_high_contrast ? "true" : "false") << '\n';
    f << "force_mono_font = " << (cfg.force_mono_font ? "true" : "false") << '\n';
    f << "keyboard_only_nav = " << (cfg.keyboard_only_nav ? "true" : "false") << '\n';
    f << "colour_blind_wong = " << (cfg.colour_blind_wong ? "true" : "false") << '\n';
    f << "reduce_motion = " << (cfg.reduce_motion ? "true" : "false") << '\n';
}

void save_to_config(const EditorA11yConfig& cfg) noexcept {
    save_to_path(cfg, a11y_file_path());
}

void apply(const EditorA11yConfig& cfg) noexcept {
    auto& theme = gw::editor::theme::ThemeRegistry::instance();
    if (cfg.force_high_contrast) {
        theme.set_active(gw::editor::theme::ThemeId::FieldTestHC);
    }
    theme.set_wong_semantic_overlay(cfg.colour_blind_wong && !cfg.force_high_contrast);
    theme.set_reduce_motion(cfg.reduce_motion);
    if (cfg.reduce_corruption) {
        theme.override_effect(gw::editor::theme::EF_Distressed, false);
        theme.override_effect(gw::editor::theme::EF_GlitchHover, false);
        theme.override_effect(gw::editor::theme::EF_CornerCracks, false);
    }
    if (cfg.disable_vignette) {
        theme.override_effect(gw::editor::theme::EF_Vignette, false);
    }

    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (cfg.keyboard_only_nav) {
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        } else {
            io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        }
    }
}

} // namespace gw::editor::a11y
