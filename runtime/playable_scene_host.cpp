// runtime/playable_scene_host.cpp — headless scene → physics (no editor link in greywater_runtime).

#include "runtime/playable_scene_host.hpp"
#include "runtime/engine.hpp"

#include "editor/scene/authoring_scene_load.hpp"
#include "editor/scene/blockout_physics_sync.hpp"
#include "editor/scene/components.hpp"
#include "engine/ecs/world.hpp"
#include "engine/scene/scene_file.hpp"

#include <filesystem>

namespace gw::runtime {

bool load_authoring_scene_into_physics(Engine&                     engine,
                                       const std::string&        path_utf8,
                                       PlayableSceneLoadSummary& out) {
    out = {};

    gw::ecs::World world;
    gw::editor::scene::ensure_authoring_scene_types_for_load(world);
    gw::editor::scene::register_authoring_scene_migrations_once();

    const std::filesystem::path p{path_utf8};
    const auto                  loaded = gw::scene::load_scene(p, world);
    if (!loaded) return false;

    std::uint32_t cooked_mesh = 0;
    world.for_each<gw::editor::scene::EditorCookedMeshComponent>(
        [&](gw::ecs::Entity /*e*/, const gw::editor::scene::EditorCookedMeshComponent& m) {
            if (m.cooked_vfs_path[0] != '\0') {
                ++cooked_mesh;
            }
        });

    out.entities        = world.entity_count();
    out.blockout_bodies      = gw::editor::scene::sync_blockout_boxes_to_physics(
        engine.physics(), world);
    out.cooked_mesh_entities = cooked_mesh;
    return true;
}

} // namespace gw::runtime
