// tests/unit/assets/cook_trust_test.cpp — cook_trust / Ed25519 (Phase 24).

#include <doctest/doctest.h>

#include "engine/assets/cook_trust.hpp"

TEST_CASE("cook_trust — Debug skips verify (returns nullopt)") {
#if !defined(NDEBUG)
    const std::uint8_t payload[] = {1, 2, 3};
    const std::uint8_t sig[64]{};
    const auto r = gw::assets::cook_trust::verify_cooked_payload_ed25519(
        payload, sig);
    CHECK_FALSE(r.has_value());
#else
    const std::uint8_t payload[] = {1, 2, 3};
    const std::uint8_t sig[64]{};
    const auto r = gw::assets::cook_trust::verify_cooked_payload_ed25519(
        payload, sig);
    REQUIRE(r.has_value());
    CHECK_FALSE(*r);
#endif
}

#if defined(NDEBUG)
TEST_CASE("cook_trust — Release smoke vector verifies") {
    CHECK(gw::assets::cook_trust::cook_trust_smoke_vector_verifies());
}

TEST_CASE("cook_trust — Release rejects wrong-length signature") {
    const std::uint8_t payload[] = {0};
    const std::uint8_t sig[32]{};
    const auto r =
        gw::assets::cook_trust::verify_cooked_payload_ed25519(payload, sig);
    REQUIRE(r.has_value());
    CHECK_FALSE(*r);
}

TEST_CASE("cook_trust — Release rejects bad signature bytes") {
    const std::uint8_t payload[] = {'g', 'w', '_', 'c', 'o', 'o', 'k', '_',
                                    't', 'r', 'u', 's', 't', '_', 'c', 'i',
                                    '_', 'v', 'e', 'c', 't', 'o', 'r'};
    const std::uint8_t sig[64]{};
    const auto r =
        gw::assets::cook_trust::verify_cooked_payload_ed25519(payload, sig);
    REQUIRE(r.has_value());
    CHECK_FALSE(*r);
}

TEST_CASE("cook_trust — verify_cooked_manifest_ed25519 alias matches payload API") {
    const std::uint8_t payload[] = {'g', 'w', '_', 'c', 'o', 'o', 'k', '_',
                                    't', 'r', 'u', 's', 't', '_', 'c', 'i',
                                    '_', 'v', 'e', 'c', 't', 'o', 'r'};
    const std::uint8_t sig[64] = {
        94,  177, 201, 220, 144, 165, 79,  94,  99,  115, 99,  7,   211, 250, 75,  9,
        219, 131, 162, 189, 187, 35,  66,  174, 114, 199, 193, 20,  42,  169, 239, 113,
        135, 194, 211, 174, 45,  114, 186, 69,  229, 101, 33,  83,  102, 109, 217, 162,
        231, 160, 156, 172, 113, 243, 228, 165, 214, 96,  198, 17,  246, 160, 111, 3,
    };
    const auto a = gw::assets::cook_trust::verify_cooked_payload_ed25519(payload, sig);
    const auto b = gw::assets::cook_trust::verify_cooked_manifest_ed25519(payload, sig);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    CHECK(*a == *b);
    CHECK(*a);
}
#endif
