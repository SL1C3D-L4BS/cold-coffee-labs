// editor/shell/franchise_roots.cpp

#include "editor/shell/franchise_roots.hpp"

#include <cctype>
#include <fstream>
#include <string_view>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace gw::editor::shell {
namespace {

[[nodiscard]] std::string trim_ascii(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return std::string{sv};
}

[[nodiscard]] bool is_repo_marker(const std::filesystem::path& p) {
    namespace fs = std::filesystem;
    std::error_code ec;
    return fs::is_regular_file(p / "franchises" / "sacrilege" / "games.manifest", ec);
}

[[nodiscard]] std::optional<std::filesystem::path> walk_for_repo(
    std::filesystem::path start) {
    namespace fs = std::filesystem;
    for (int i = 0; i < 24; ++i) {
        if (is_repo_marker(start)) return start;
        if (!start.has_parent_path()) break;
        const fs::path parent = start.parent_path();
        if (parent == start) break;
        start = parent;
    }
    return std::nullopt;
}

#ifdef _WIN32
[[nodiscard]] std::optional<std::filesystem::path> current_executable_dir() noexcept {
    wchar_t buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return std::nullopt;
    std::filesystem::path p{buf};
    if (p.has_parent_path()) return p.parent_path();
    return std::nullopt;
}
#endif

} // namespace

std::optional<std::filesystem::path> find_greywater_repo_root() noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;

    if (auto cwd = walk_for_repo(fs::current_path(ec)); cwd) return cwd;

#ifdef _WIN32
    if (auto exe_dir = current_executable_dir()) {
        if (auto from_exe = walk_for_repo(*exe_dir); from_exe) return from_exe;
    }
#endif
    return std::nullopt;
}

std::vector<FranchiseGameEntry> list_sacrilege_franchise_games(
    const std::filesystem::path& repo_root) noexcept {
    namespace fs = std::filesystem;
    std::vector<FranchiseGameEntry> out;
    const fs::path manifest_path =
        repo_root / "franchises" / "sacrilege" / "games.manifest";
    std::ifstream f(manifest_path);
    if (!f) return out;

    const fs::path franchise_home = repo_root / "franchises" / "sacrilege";

    std::string line;
    while (std::getline(f, line)) {
        const std::string trimmed = trim_ascii(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        const auto tab = trimmed.find('\t');
        if (tab == std::string::npos) continue;
        const std::string slug = trim_ascii(std::string_view{trimmed.data(), tab});
        const std::string display =
            trim_ascii(std::string_view{trimmed.data() + tab + 1,
                                        trimmed.size() - tab - 1});
        if (slug.empty() || display.empty()) continue;

        const fs::path game_root = franchise_home / slug;
        std::error_code ec;
        if (!fs::is_directory(game_root, ec)) continue;

        const fs::path abs_root = fs::absolute(game_root, ec);
        out.push_back(FranchiseGameEntry{
            .root         = abs_root,
            .slug         = slug,
            .display_name = display,
        });
    }
    return out;
}

} // namespace gw::editor::shell
