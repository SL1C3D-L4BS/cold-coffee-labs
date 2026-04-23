// editor/config/editor_paths.cpp

#include "editor/config/editor_paths.hpp"

#include <cstdlib>

namespace gw::editor::config {

std::filesystem::path editor_data_root() noexcept {
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"); appdata)
        return std::filesystem::path{appdata} / "GreywaterEditor";
#else
    if (const char* home = std::getenv("HOME"); home)
        return std::filesystem::path{home} / ".config" / "greywater_editor";
#endif
    return std::filesystem::path{"."} / "greywater_editor_data";
}

} // namespace gw::editor::config
