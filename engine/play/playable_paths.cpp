// engine/play/playable_paths.cpp

#include "engine/play/playable_paths.hpp"

#include <string>

namespace gw::play {

std::string play_cvars_rel_for_scene_path(std::string_view scene_rel_generic) {
    const auto slash = scene_rel_generic.find_last_of("/\\");
    const std::size_t dir_end =
        (slash == std::string_view::npos) ? 0 : (slash + 1);
    const std::size_t dot = scene_rel_generic.find_last_of('.');
    std::string       stem;
    if (dot == std::string_view::npos || dot < dir_end) {
        stem = std::string(scene_rel_generic.substr(dir_end));
    } else {
        stem = std::string(scene_rel_generic.substr(dir_end, dot - dir_end));
    }
    if (dir_end == 0) {
        return stem + ".play_cvars.toml";
    }
    return std::string(scene_rel_generic.substr(0, slash)) + "/" + stem +
           ".play_cvars.toml";
}

} // namespace gw::play
