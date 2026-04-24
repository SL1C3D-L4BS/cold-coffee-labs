#pragma once
// editor/scene/authoring_scene_load.hpp — shared .gwscene registration for editor + headless runtimes.

#include "engine/ecs/world.hpp"

namespace gw::editor::scene {

/// Seed ComponentRegistry with authoring component types (idempotent).
void ensure_authoring_scene_types_for_load(gw::ecs::World& w);

/// Register Transform + Blockout migrations once (idempotent).
void register_authoring_scene_migrations_once();

} // namespace gw::editor::scene
