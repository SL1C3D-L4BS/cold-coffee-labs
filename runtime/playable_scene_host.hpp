#pragma once

#include <cstdint>
#include <string>

namespace gw::runtime {

class Engine;

struct PlayableSceneLoadSummary {
    std::uint32_t entities         = 0;
    std::uint32_t blockout_bodies  = 0;
};

/// Load authoring `.gwscene` into a scratch `World`, sync blockout boxes to
/// `engine.physics()`, then discard ECS (scene data is validated + physics
/// populated for the session). Returns false on I/O / codec failure.
[[nodiscard]] bool load_authoring_scene_into_physics(Engine&                     engine,
                                                     const std::string&        path_utf8,
                                                     PlayableSceneLoadSummary& out);

} // namespace gw::runtime
