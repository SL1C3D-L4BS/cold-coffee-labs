// tools/cook/content_signing.cpp — Part C §22 scaffold.
//
// Release runtime verification lives in `engine/assets/cook_trust.cpp`
// (`verify_cooked_manifest_ed25519`). When signing is implemented, sign the
// exact serialized manifest bytes the runtime will verify (same domain as ADR-0096).

#include "tools/cook/content_signing.hpp"

namespace gw::cook::signing {

bool sign_manifest(std::string_view /*manifest_path*/,
                   std::string_view /*private_key_path*/,
                   Signature& /*out_sig*/) noexcept {
    // Phase 22: BLAKE3 + ed25519 over the manifest; ship via a cooked
    // `.sig` sidecar. Scaffold leaves out_sig zeroed.
    return false;
}

bool verify_manifest(std::string_view /*manifest_path*/,
                     const PublicKey& /*pub*/,
                     const Signature& /*sig*/) noexcept {
    // Phase 22: reject unsigned / mismatched cooks in Release builds; warn
    // in Dev / Debug. Scaffold returns true so current dev flow is
    // unaffected.
    return true;
}

} // namespace gw::cook::signing
