#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace gw::editor::shell {

struct RecentProject {
    std::filesystem::path root;
    std::string           display_name;
    std::int64_t          last_opened_unix = 0;
    bool                  pinned          = false;
};

[[nodiscard]] std::vector<RecentProject> load_recent_projects() noexcept;

void save_recent_projects(const std::vector<RecentProject>& items) noexcept;

void sort_recents_for_display(std::vector<RecentProject>& items) noexcept;

void touch_recent_project(std::vector<RecentProject>& items,
                          const std::filesystem::path& root) noexcept;

void set_recent_pinned(std::vector<RecentProject>& items,
                       const std::filesystem::path& root, bool pinned) noexcept;

void remove_recent_project(std::vector<RecentProject>& items,
                           const std::filesystem::path& root) noexcept;

[[nodiscard]] bool is_likely_project_root(const std::filesystem::path& root) noexcept;

} // namespace gw::editor::shell
