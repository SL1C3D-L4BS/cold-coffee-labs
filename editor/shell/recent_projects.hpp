#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace gw::editor::shell {

struct RecentProject {
    std::filesystem::path root;
    std::string           display_name;
};

[[nodiscard]] std::vector<RecentProject> load_recent_projects() noexcept;

void save_recent_projects(const std::vector<RecentProject>& items) noexcept;

void touch_recent_project(std::vector<RecentProject>& items,
                          const std::filesystem::path& root) noexcept;

} // namespace gw::editor::shell
