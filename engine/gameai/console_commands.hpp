#pragma once
// engine/gameai/console_commands.hpp — Phase 13 (ADR-0043, ADR-0044).
//
//   ai.pause  [on|off]               — toggle BT / agent simulation pause
//   ai.step   [N]                    — step N BT+agent fixed frames
//   ai.hash                          — print combined determinism hash
//   ai.nav.hash                      — print navmesh content hash
//   ai.bt.hash                       — print BT state hash
//   ai.debug <flags|off>             — set debug draw flag mask
//       (nav_tiles | nav_polys | paths | agents | bt_active | blackboard | all | off)

#include "engine/console/console_service.hpp"
#include "engine/gameai/gameai_world.hpp"

namespace gw::gameai {

void register_gameai_console_commands(console::ConsoleService& svc,
                                      GameAIWorld& world);

} // namespace gw::gameai
