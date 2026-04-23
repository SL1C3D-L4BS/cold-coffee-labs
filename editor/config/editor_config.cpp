// editor/config/editor_config.cpp — minimal editor.toml (theme key only).

#include "editor/config/editor_config.hpp"
#include "editor/config/editor_paths.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

namespace gw::editor::config {
namespace {

[[nodiscard]] std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

[[nodiscard]] const char* theme_id_string(gw::editor::theme::ThemeId id) noexcept {
    using gw::editor::theme::ThemeId;
    switch (id) {
    case ThemeId::CorruptedRelic: return "corrupted_relic";
    case ThemeId::FieldTestHC:    return "field_test_hc";
    case ThemeId::BrewedSlate:
    default:                      return "brewed_slate";
    }
}

[[nodiscard]] gw::editor::theme::ThemeId parse_theme(std::string_view v) noexcept {
    using gw::editor::theme::ThemeId;
    if (v == "corrupted_relic") return ThemeId::CorruptedRelic;
    if (v == "field_test_hc")   return ThemeId::FieldTestHC;
    if (v == "brewed_slate")   return ThemeId::BrewedSlate;
    return ThemeId::BrewedSlate;
}

} // namespace

gw::editor::theme::ThemeId load_saved_theme(
    gw::editor::theme::ThemeId fallback) noexcept {
    namespace fs = std::filesystem;
    const fs::path root = editor_data_root();
    std::error_code ec;
    fs::create_directories(root, ec);
    const fs::path cfg = root / "editor.toml";
    std::ifstream in{cfg};
    if (!in.good()) return fallback;

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        constexpr std::string_view key = "theme";
        if (line.size() < key.size()) continue;
        if (line.compare(0, key.size(), key) != 0) continue;
        std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string val = trim(line.substr(eq + 1));
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }
        return parse_theme(val);
    }
    return fallback;
}

void save_theme(gw::editor::theme::ThemeId id) noexcept {
    namespace fs = std::filesystem;
    const fs::path root = editor_data_root();
    std::error_code ec;
    fs::create_directories(root, ec);
    const fs::path cfg = root / "editor.toml";
    std::ofstream out{cfg, std::ios::trunc};
    if (!out.good()) return;
    out << "# Greywater Editor — persisted preferences\n"
        << "theme = \"" << theme_id_string(id) << "\"\n";
}

} // namespace gw::editor::config
