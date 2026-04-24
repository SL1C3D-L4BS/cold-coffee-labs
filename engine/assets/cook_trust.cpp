// engine/assets/cook_trust.cpp — Ed25519 verify for cooked payloads (ADR-0096, ADR-0115; Phase 24).
//
// Debug: verification skipped (returns nullopt). Release: verifies against one
// or more pinned public keys (any match succeeds). Align signing in
// `tools/cook/content_signing.cpp` with the same message bytes once manifests ship.

#include "engine/assets/cook_trust.hpp"

#if defined(NDEBUG)
#include "ed25519.h"

#include <array>
#endif

namespace gw::assets::cook_trust {
namespace {

#if defined(NDEBUG)
// Pinned public keys for the deterministic seed used only to generate the
// `kSmokePayload` / `kSmokeSignature` test vector (orlp/ed25519, seed 0x3c×32).
// Slot [1] is intentionally wrong for that vector — exercises multi-key iteration.
constexpr std::array<std::array<std::uint8_t, 32>, 2> kPinnedPublicKeysEd25519 = {{
    {{
        85,  38,  247, 66,  148, 23,  17,  179, 188, 83,  11,  164, 79,  246, 246, 218,
        176, 240, 171, 113, 175, 131, 47,  65,  167, 254, 59,  159, 218, 237, 156, 96,
    }},
    {{
        // Secondary pin slot (CI/prod can embed additional trusted keys).
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    }},
}};

constexpr std::uint8_t kSmokePayload[] = {
    'g', 'w', '_', 'c', 'o', 'o', 'k', '_', 't', 'r', 'u', 's', 't', '_', 'c', 'i',
    '_', 'v', 'e', 'c', 't', 'o', 'r',
};

constexpr std::uint8_t kSmokeSignatureEd25519[64] = {
    94,  177, 201, 220, 144, 165, 79,  94,  99,  115, 99,  7,   211, 250, 75,  9,
    219, 131, 162, 189, 187, 35,  66,  174, 114, 199, 193, 20,  42,  169, 239, 113,
    135, 194, 211, 174, 45,  114, 186, 69,  229, 101, 33,  83,  102, 109, 217, 162,
    231, 160, 156, 172, 113, 243, 228, 165, 214, 96,  198, 17,  246, 160, 111, 3,
};
#endif

} // namespace

std::optional<bool> verify_cooked_payload_ed25519(
    std::span<const std::uint8_t> payload,
    std::span<const std::uint8_t> signature_ed25519_64) noexcept {
#if !defined(NDEBUG)
    (void)payload;
    (void)signature_ed25519_64;
    return std::nullopt;
#else
    if (signature_ed25519_64.size() != 64) {
        return false;
    }
    for (const auto& pk : kPinnedPublicKeysEd25519) {
        const int ok = ed25519_verify(
            reinterpret_cast<const unsigned char*>(signature_ed25519_64.data()),
            reinterpret_cast<const unsigned char*>(payload.data()),
            payload.size(),
            pk.data());
        if (ok == 1) {
            return true;
        }
    }
    return false;
#endif
}

#if defined(NDEBUG)
bool cook_trust_smoke_vector_verifies() noexcept {
    return ed25519_verify(
               kSmokeSignatureEd25519,
               kSmokePayload,
               sizeof(kSmokePayload),
               kPinnedPublicKeysEd25519[0].data()) == 1;
}
#endif

} // namespace gw::assets::cook_trust
