// tools/cook/content_signing.cpp — Part C §22 scaffold.

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
