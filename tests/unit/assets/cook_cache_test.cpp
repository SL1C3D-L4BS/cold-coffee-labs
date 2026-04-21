// tests/unit/assets/cook_cache_test.cpp
// Phase 6 spec §13.1: cook key stability and sensitivity tests.
// Runs without Vulkan or GPU — pure CPU logic.
#include <doctest/doctest.h>
#include "engine/assets/asset_types.hpp"
#include <xxhash.h>
#include <cstring>

using gw::assets::CookKey;

// Compute xxHash3-128 on arbitrary bytes.
static CookKey compute_key(const void* data, size_t size,
                            uint32_t rule_version,
                            uint8_t  platform,
                            uint8_t  config)
{
    XXH3_state_t* s = XXH3_createState();
    XXH3_128bits_reset(s);
    XXH3_128bits_update(s, data, size);
    XXH3_128bits_update(s, &rule_version, sizeof(rule_version));
    XXH3_128bits_update(s, &platform, 1);
    XXH3_128bits_update(s, &config,   1);
    XXH128_hash_t h = XXH3_128bits_digest(s);
    XXH3_freeState(s);
    return CookKey{h.high64, h.low64};
}

TEST_CASE("cook key stability — same input = same key") {
    const uint8_t data[]    = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint32_t rule_ver = 1u;
    const uint8_t  plat     = 0u;
    const uint8_t  cfg      = 1u;

    CookKey k1 = compute_key(data, sizeof(data), rule_ver, plat, cfg);
    CookKey k2 = compute_key(data, sizeof(data), rule_ver, plat, cfg);
    CHECK(k1 == k2);
}

TEST_CASE("cook key sensitivity — flip 1 byte = different key") {
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    CookKey k1 = compute_key(data, sizeof(data), 1u, 0u, 1u);

    data[3] ^= 0x01; // flip one bit
    CookKey k2 = compute_key(data, sizeof(data), 1u, 0u, 1u);
    CHECK(k1 != k2);
}

TEST_CASE("cook key sensitivity — different rule version = different key") {
    const uint8_t data[] = {10, 20, 30};
    CookKey k1 = compute_key(data, sizeof(data), 1u, 0u, 1u);
    CookKey k2 = compute_key(data, sizeof(data), 2u, 0u, 1u);
    CHECK(k1 != k2);
}

TEST_CASE("cook key sensitivity — different platform = different key") {
    const uint8_t data[] = {10, 20, 30};
    CookKey k1 = compute_key(data, sizeof(data), 1u, 0u, 1u);
    CookKey k2 = compute_key(data, sizeof(data), 1u, 1u, 1u);
    CHECK(k1 != k2);
}
