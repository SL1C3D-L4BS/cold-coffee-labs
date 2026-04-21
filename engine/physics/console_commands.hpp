#pragma once
// engine/physics/console_commands.hpp — Phase 12 (ADR-0031, ADR-0036, ADR-0037).
//
// Register physics-flavoured console built-ins:
//   physics.pause  [on|off]          — toggle / set pause
//   physics.step   [N]               — advance N fixed steps (default 1)
//   physics.hash                     — print current determinism hash
//   physics.debug  <flags|off>       — set debug draw flag mask (bodies,
//                                      contacts, broadphase, character,
//                                      constraints, queries, all, off)

#include "engine/console/console_service.hpp"
#include "engine/physics/physics_world.hpp"

namespace gw::physics {

void register_physics_console_commands(console::ConsoleService& svc,
                                       PhysicsWorld& world);

} // namespace gw::physics
