// editor/shell/recent_projects.cpp — pipe-delimited rows in app data.

#include "editor/shell/recent_projects.hpp"
#include "editor/config/editor_paths.hpp"

#include <fstream>
#include <unordered_set>

namespace gw::editor::shell {
namespace {

constexpr std::size_t kMaxRecents = 16;

[[nodiscard]] std::string escape_path(const std::filesystem::path& p) {
    std::string s = p.generic_string();
    for (char& c : s)
        if (c == '|') c = '\\';
    return s;
}

[[nodiscard]] std::filesystem::path unescape_path(std::string s) {
    for (char& c : s)
        if (c == '\\') c = '|';
    return std::filesystem::path{s};
}

} // namespace

std::vector<RecentProject> load_recent_projects() noexcept {
    namespace fs = std::filesystem;
    std::vector<RecentProject> out;
    const fs::path file = config::editor_data_root() / "recent_projects.txt";
    std::ifstream in{file};
    if (!in.good()) return out;
    std::string line;
    while (std::getline(in, line) && out.size() < kMaxRecents) {
        if (line.empty()) continue;
        auto pipe = line.find('|');
        RecentProject r{};
        if (pipe == std::string::npos) {
            r.root = unescape_path(line);
            r.display_name = r.root.filename().string();
        } else {
            r.root = unescape_path(line.substr(0, pipe));
            r.display_name = line.substr(pipe + 1);
        }
        if (!r.root.empty()) out.push_back(std::move(r));
    }
    return out;
}

void save_recent_projects(const std::vector<RecentProject>& items) noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(config::editor_data_root(), ec);
    const fs::path file = config::editor_data_root() / "recent_projects.txt";
    std::ofstream out{file, std::ios::trunc};
    if (!out.good()) return;
    for (const auto& r : items) {
        out << escape_path(r.root) << '|' << r.display_name << '\n';
    }
}

void touch_recent_project(std::vector<RecentProject>& items,
                          const std::filesystem::path& root) noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path canon = fs::weakly_canonical(root, ec);
    const std::string key = canon.generic_string();

    std::vector<RecentProject> next;
    next.reserve(items.size() + 1);
    RecentProject head{canon, canon.filename().string()};
    next.push_back(head);

    std::unordered_set<std::string> seen;
    seen.insert(key);
    for (const auto& r : items) {
        const std::string k = r.root.generic_string();
        if (seen.count(k)) continue;
        seen.insert(k);
        next.push_back(r);
        if (next.size() >= kMaxRecents) break;
    }
    items = std::move(next);
    save_recent_projects(items);
}

} // namespace gw::editor::shell
