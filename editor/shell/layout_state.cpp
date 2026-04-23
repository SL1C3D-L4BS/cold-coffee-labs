// editor/shell/layout_state.cpp

#include "editor/shell/layout_state.hpp"

#include <cctype>
#include <fstream>
#include <string>

namespace gw::editor::shell {
namespace {

[[nodiscard]] std::filesystem::path app_layout_dir() {
    namespace fs = std::filesystem;
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"); appdata)
        return fs::path{appdata} / "GreywaterEditor";
#else
    if (const char* home = std::getenv("HOME"); home)
        return fs::path{home} / ".config" / "greywater_editor";
#endif
    return fs::path{"."};
}

// Minimum window set for a valid saved layout (floating-first; optional panels may be absent).
constexpr const char* kCriticalWindowTitles[] = {
    "Viewport",
};

[[nodiscard]] bool contains_window_section(std::string_view ini,
                                             std::string_view title) {
    // ImGui serialises as `[Window][Title]`
    std::string needle;
    needle.reserve(title.size() + 12);
    needle.append("[Window][");
    needle.append(title);
    needle.push_back(']');
    return ini.find(needle) != std::string_view::npos;
}

} // namespace

std::filesystem::path layout_ini_directory() noexcept {
    return app_layout_dir();
}

std::filesystem::path layout_ini_path() noexcept {
    return app_layout_dir() / "layout.ini";
}

std::filesystem::path layout_schema_path() noexcept {
    return app_layout_dir() / "layout_schema.txt";
}

int read_layout_schema_version() noexcept {
    std::ifstream in{layout_schema_path()};
    if (!in.good()) return -1;
    std::string line;
    if (!std::getline(in, line)) return -1;
    // Trim
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front())))
        line.erase(line.begin());
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back())))
        line.pop_back();
    if (line.empty()) return -1;
    try {
        return std::stoi(line);
    } catch (...) {
        return -1;
    }
}

void write_layout_schema_version(int version) noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(layout_ini_directory(), ec);
    std::ofstream out{layout_schema_path(), std::ios::trunc};
    if (!out.good()) return;
    out << version << '\n';
}

bool layout_ini_has_critical_windows(std::string_view ini) noexcept {
    if (ini.find("[Docking]") == std::string_view::npos)
        return false;
    for (const char* title : kCriticalWindowTitles) {
        if (!contains_window_section(ini, title))
            return false;
    }
    return true;
}

} // namespace gw::editor::shell
