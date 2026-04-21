#pragma once
// engine/net/console_commands.hpp — Phase 14 Wave 14K.
//
//   net.listen [port]                — bind the server transport
//   net.connect <host> [port]        — open a client session to host
//   net.disconnect <peer_id>         — tear down a peer
//   net.stats                        — print transport + reliability stats
//   net.snapshot                     — publish a snapshot round for all peers
//   net.desync.dump                  — force-dump the desync ring
//   net.voice.mute <peer_id>         — toggle mute on a peer
//   net.sync_test [frames]           — run LockstepSession::run_sync_test

#include "engine/console/console_service.hpp"
#include "engine/net/network_world.hpp"

namespace gw::net {

void register_network_console_commands(console::ConsoleService& svc,
                                       NetworkWorld& world);

} // namespace gw::net
