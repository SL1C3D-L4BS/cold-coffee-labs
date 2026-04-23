#pragma once
// tools/cook/content_signing.hpp — Part C §22 scaffold (ADR-0115).
//
// Ed25519 signing of cooked assets. The cook worker stamps every asset
// manifest with a signature; Release-build loaders reject unsigned or
// mismatched cooks at runtime. Dev / Debug builds warn but accept.

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace gw::cook::signing {

constexpr std::size_t ED25519_SIGNATURE_BYTES = 64;
constexpr std::size_t ED25519_PRIVATE_KEY_BYTES = 64;
constexpr std::size_t ED25519_PUBLIC_KEY_BYTES = 32;
constexpr std::size_t BLAKE3_DIGEST_BYTES      = 32;

struct Signature {
    std::uint8_t bytes[ED25519_SIGNATURE_BYTES]{};
};

struct PublicKey {
    std::uint8_t bytes[ED25519_PUBLIC_KEY_BYTES]{};
};

/// Sign a manifest file (already-hashed with BLAKE3 + xxhash3 in place).
/// `private_key_path` points to the signing key mounted from the sealed
/// CI secret. Returns false on I/O error.
bool sign_manifest(std::string_view manifest_path,
                   std::string_view private_key_path,
                   Signature& out_sig) noexcept;

/// Verify a manifest signature. Used by the Release-build asset loader.
bool verify_manifest(std::string_view manifest_path,
                     const PublicKey& pub,
                     const Signature& sig) noexcept;

} // namespace gw::cook::signing
