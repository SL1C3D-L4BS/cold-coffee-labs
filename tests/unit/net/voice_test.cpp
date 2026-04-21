// tests/unit/net/voice_test.cpp — Phase 14 Wave 14I.

#include <doctest/doctest.h>

#include "engine/net/transport.hpp"
#include "engine/net/voice.hpp"

#include <array>
#include <vector>

using namespace gw::net;

TEST_CASE("VoicePipeline routes an encoded frame to the transport") {
    auto server = make_null_transport();
    REQUIRE(server->listen(30050));
    auto client = make_null_transport();
    const PeerId srv = client->connect("127.0.0.1", 30050);
    VoicePipeline vp{VoiceConfig{}};
    const std::array<std::int16_t, 16> pcm{};
    vp.push_local_frame(std::span<const std::int16_t>(pcm), *client, srv);
    CHECK(vp.frames_sent() == 1);
    std::vector<Packet> inbound;
    (void)server->poll(inbound);
    CHECK(inbound.size() >= 1);
    CHECK(inbound.front().channel == kChannelVoice);
}

TEST_CASE("VoicePipeline on_inbound dedups and honours mute mask") {
    VoicePipeline vp{VoiceConfig{}};
    const PeerId peer{3};
    const std::uint8_t bytes[4] = {1, 2, 3, 4};
    CHECK(vp.on_inbound(peer, 1, std::span<const std::uint8_t>(bytes, 4)));
    CHECK(vp.on_inbound(peer, 2, std::span<const std::uint8_t>(bytes, 4)));
    CHECK_FALSE(vp.on_inbound(peer, 2, std::span<const std::uint8_t>(bytes, 4))); // dup
    // Mute and ensure further frames drop.
    vp.set_mute_mask(std::uint64_t{1} << peer.id);
    CHECK_FALSE(vp.on_inbound(peer, 3, std::span<const std::uint8_t>(bytes, 4)));
    CHECK(vp.frames_dropped() >= 2);
}

TEST_CASE("VoicePipeline drain_decoded clears internal ring") {
    VoicePipeline vp{VoiceConfig{}};
    const PeerId peer{4};
    const std::uint8_t b[1] = {42};
    CHECK(vp.on_inbound(peer, 1, std::span<const std::uint8_t>(b, 1)));
    std::vector<VoiceFrame> out;
    vp.drain_decoded(out);
    CHECK(out.size() == 1);
    std::vector<VoiceFrame> out2;
    vp.drain_decoded(out2);
    CHECK(out2.empty());
}
