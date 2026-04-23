#pragma once

#include <cstdint>
#include <string_view>

namespace gw::editor::shell {

/// RFC 6234 style SHA-256; `out_digest` is 32 bytes.
void sha256(std::string_view data, std::uint8_t out_digest[32]) noexcept;

[[nodiscard]] bool digest_to_hex(const std::uint8_t digest[32],
                                 char out_hex_65[65]) noexcept;

} // namespace gw::editor::shell
