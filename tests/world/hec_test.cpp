#include <doctest/doctest.h>

#include "engine/core/crypto/sha3.hpp"
#include "engine/world/universe/hec.hpp"

#include <array>

using gw::crypto::sha3_256_digest;
using gw::universe::HecDomain;
using gw::universe::Pcg64Rng;
using gw::universe::UniverseSeed;
using gw::universe::hec_derive;
using gw::universe::hec_to_rng;
using gw::universe::hec_to_u64;

TEST_CASE("sha3_256 empty string matches NIST") {
    static constexpr std::array<std::uint8_t, 32> k_expected = {
        0xa7, 0xff, 0xc6, 0xf8, 0xbf, 0x1e, 0xd7, 0x66, 0x51, 0xc1, 0x47, 0x56, 0xa0, 0x61, 0xd6, 0x62,
        0xf5, 0x80, 0xff, 0x4d, 0xe4, 0x3b, 0x49, 0xfa, 0x82, 0xd8, 0x0a, 0x4b, 0x80, 0xf8, 0x43, 0x4a};
    const auto h = sha3_256_digest({});
    CHECK(h == k_expected);
}

TEST_CASE("hec_derive Greywater Arena (0,0,0) is stable cross-platform") {
    static constexpr std::array<std::uint8_t, 32> k_expected = {
        0x65, 0x47, 0x6a, 0x17, 0xf6, 0xa0, 0xdc, 0xe1, 0xfc, 0xfe, 0x3b, 0xa5, 0x8e, 0x85, 0x2b, 0x80,
        0xb9, 0x31, 0x7c, 0x4a, 0x10, 0x71, 0x85, 0x65, 0xc5, 0x5a, 0xc6, 0x9c, 0x4d, 0x19, 0x24, 0xcf};

    const UniverseSeed               parent("Greywater");
    const UniverseSeed               child = hec_derive(parent, HecDomain::Arena, 0, 0, 0);
    static constexpr std::array<std::uint8_t, 32> k_parent_sha256 = {
        0xfd, 0x1c, 0x8f, 0x11, 0xd1, 0x32, 0xc4, 0x8c, 0x7a, 0x86, 0xce, 0xd3, 0xed, 0x98, 0xa2, 0xbb,
        0xc4, 0x9c, 0x0c, 0x98, 0x9b, 0x5e, 0xfa, 0x98, 0x4f, 0x53, 0x18, 0x0a, 0x6c, 0x40, 0x9d, 0xe6};
    CHECK(parent.bytes == k_parent_sha256);
    CHECK(child.bytes == k_expected);
}

TEST_CASE("hec_derive different domains diverge") {
    const UniverseSeed p("Greywater");
    const UniverseSeed a = hec_derive(p, HecDomain::Arena, 0, 0, 0);
    const UniverseSeed b = hec_derive(p, HecDomain::Chunk, 0, 0, 0);
    CHECK(a.bytes != b.bytes);
}

TEST_CASE("hec_derive is stable over repeated invocations") {
    const UniverseSeed p("Greywater");
    for (int i = 0; i < 1000; ++i) {
        const UniverseSeed a = hec_derive(p, HecDomain::Loot, 7, -3, 42);
        const UniverseSeed b = hec_derive(p, HecDomain::Loot, 7, -3, 42);
        CHECK(a.bytes == b.bytes);
    }
}

TEST_CASE("pcg64 sequence is deterministic from hec_to_rng") {
    const UniverseSeed u("Greywater");
    const UniverseSeed d = hec_derive(u, HecDomain::Enemy, 1, 2, 3);
    Pcg64Rng           a = hec_to_rng(d);
    Pcg64Rng           b = hec_to_rng(d);
    for (int i = 0; i < 64; ++i) {
        CHECK(a.next_u64() == b.next_u64());
    }
}

TEST_CASE("hec_to_u64 is stable") {
    const UniverseSeed u("Greywater");
    const std::uint64_t x = hec_to_u64(u);
    for (int i = 0; i < 100; ++i) {
        CHECK(hec_to_u64(u) == x);
    }
}
