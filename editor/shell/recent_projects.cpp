// editor/shell/recent_projects.cpp — pipe-delimited rows in app data.

#include "editor/shell/recent_projects.hpp"
#include "editor/config/editor_paths.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <string_view>
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

[[nodiscard]] std::int64_t parse_epoch(std::string_view sv) {
    if (sv.empty()) return 0;
    try {
        return static_cast<std::int64_t>(std::stoll(std::string{sv}));
    } catch (...) {
        return 0;
    }
}

[[nodiscard]] bool parse_bool01(std::string_view sv) {
    return sv == "1" || sv == "true" || sv == "True";
}

} // namespace

void sort_recents_for_display(std::vector<RecentProject>& items) noexcept {
    std::stable_sort(items.begin(), items.end(),
        [](const RecentProject& a, const RecentProject& b) {
            if (a.pinned != b.pinned) return a.pinned > b.pinned;
            return a.last_opened_unix > b.last_opened_unix;
        });
}

bool is_likely_project_root(const std::filesystem::path& root) noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) return false;
    return fs::is_directory(root / "content", ec) || fs::is_directory(root / "assets", ec) ||
           fs::is_directory(root / "franchises", ec);
}

std::vector<RecentProject> load_recent_projects() noexcept {
    namespace fs = std::filesystem;
    std::vector<RecentProject> out;
    const fs::path file = config::editor_data_root() / "recent_projects.txt";
    std::ifstream in{file};
    if (!in.good()) return out;
    std::string line;
    while (std::getline(in, line) && out.size() < kMaxRecents) {
        if (line.empty()) continue;
        RecentProject r{};
        std::size_t p0 = line.find('|');
        if (p0 == std::string::npos) {
            r.root         = unescape_path(line);
            r.display_name = r.root.filename().string();
        } else {
            r.root = unescape_path(line.substr(0, p0));
            std::size_t rest_start = p0 + 1;
            std::size_t p1       = line.find('|', rest_start);
            if (p1 == std::string::npos) {
                r.display_name = line.substr(rest_start);
            } else {
                r.display_name = line.substr(rest_start, p1 - rest_start);
                std::size_t p2 = line.find('|', p1 + 1);
                if (p2 == std::string::npos) {
                    r.last_opened_unix = parse_epoch(std::string_view{line}.substr(p1 + 1));
                } else {
                    r.last_opened_unix = parse_epoch(std::string_view{line}.substr(p1 + 1, p2 - p1 - 1));
                    r.pinned          = parse_bool01(std::string_view{line}.substr(p2 + 1));
                }
            }
        }
        if (!r.root.empty()) out.push_back(std::move(r));
    }
    sort_recents_for_display(out);
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
        out << escape_path(r.root) << '|' << r.display_name << '|' << r.last_opened_unix << '|'
            << (r.pinned ? '1' : '0') << '\n';
    }
}

void touch_recent_project(std::vector<RecentProject>& items,
                          const std::filesystem::path& root) noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path canon = fs::weakly_canonical(root, ec);
    const std::string key = canon.generic_string();

    bool         was_pinned = false;
    std::string  disp       = canon.filename().string();
    for (const auto& r : items) {
        if (r.root.generic_string() == key) {
            was_pinned = r.pinned;
            if (!r.display_name.empty()) disp = r.display_name;
            break;
        }
    }

    std::vector<RecentProject> next;
    next.reserve(items.size() + 1);
    next.push_back(RecentProject{canon, disp, std::time(nullptr), was_pinned});

    std::unordered_set<std::string> seen;
    seen.insert(key);
    for (const auto& r : items) {
        const std::string k = r.root.generic_string();
        if (seen.count(k)) continue;
        seen.insert(k);
        next.push_back(r);
        if (next.size() >= kMaxRecents) break;
    }
    sort_recents_for_display(next);
    items = std::move(next);
    save_recent_projects(items);
}

void set_recent_pinned(std::vector<RecentProject>& items,
                       const std::filesystem::path& root, bool pinned) noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path canon = fs::weakly_canonical(root, ec);
    const std::string key = canon.generic_string();
    for (auto& r : items) {
        if (r.root.generic_string() == key) {
            r.pinned = pinned;
            sort_recents_for_display(items);
            save_recent_projects(items);
            return;
        }
    }
}

void remove_recent_project(std::vector<RecentProject>& items,
                           const std::filesystem::path& root) noexcept {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path canon = fs::weakly_canonical(root, ec);
    const std::string key = canon.generic_string();
    items.erase(std::remove_if(items.begin(), items.end(),
                  [&](const RecentProject& r) { return r.root.generic_string() == key; }),
                items.end());
    save_recent_projects(items);
}

} // namespace gw::editor::shell
