#pragma once
// engine/anim/console_commands.hpp — Phase 13 (ADR-0039).
//
// Register animation-flavoured console built-ins:
//   anim.pause  [on|off]          — toggle / set pause
//   anim.step   [N]               — advance N fixed steps (default 1)
//   anim.hash                     — print current determinism hash
//   anim.debug  <flags|off>       — set debug draw flag mask
//                                   (skeletons, ik_goals, root_motion, morph, all, off)

#include "engine/anim/animation_world.hpp"
#include "engine/console/console_service.hpp"

namespace gw::anim {

void register_anim_console_commands(console::ConsoleService& svc,
                                    AnimationWorld& world);

} // namespace gw::anim
