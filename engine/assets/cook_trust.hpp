#pragma once
// engine/assets/cook_trust.hpp — Release verification for cooked blobs.
//
// Policy: ADR-0096 (cooked asset trust) and ADR-0115 (signing / key rotation).
// Narrative index: `docs/10_APPENDIX_ADRS_AND_REFERENCES.md`.
//
// Debug: verify_cooked_payload_ed25519 returns nullopt (skipped).
// Release: Ed25519 verify against pinned keys; returns true/false.

#include <cstdint>
#include <optional>
#include <span>

namespace gw::assets::cook_trust {

/// `signature` is 64 bytes Ed25519 over `payload` when verification is enabled.
/// When cook emits signed manifests, pass the same serialized manifest octets the
/// signer hashed (see `tools/cook/content_signing.cpp`).
[[nodiscard]] std::optional<bool> verify_cooked_payload_ed25519(
    std::span<const std::uint8_t> payload,
    std::span<const std::uint8_t> signature_ed25519_64) noexcept;

/// Alias for manifest-shaped payloads (same verify path until a detached schema ships).
[[nodiscard]] inline std::optional<bool> verify_cooked_manifest_ed25519(
    std::span<const std::uint8_t> manifest_serialized_bytes,
    std::span<const std::uint8_t> signature_ed25519_64) noexcept {
    return verify_cooked_payload_ed25519(manifest_serialized_bytes, signature_ed25519_64);
}

#if defined(NDEBUG)
/// Sanity-check pinned vector (CTest Release).
[[nodiscard]] bool cook_trust_smoke_vector_verifies() noexcept;
#endif

} // namespace gw::assets::cook_trust
