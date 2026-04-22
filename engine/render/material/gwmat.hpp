#pragma once
// engine/render/material/gwmat.hpp — ADR-0076 .gwmat v1 codec.

#include "engine/render/material/gwmat_format.hpp"
#include "engine/render/material/material_types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace gw::render::material {

class MaterialTemplate;
class MaterialInstance;

struct GwMatBlob {
    std::vector<std::byte> bytes;
    std::uint64_t          content_hash{0};
};

// Encode a fresh instance into a .gwmat v1 blob. Deterministic: same
// inputs produce identical bytes on every platform.
[[nodiscard]] GwMatBlob encode_gwmat(const MaterialInstanceState& state,
                                      const MaterialTemplate&     tmpl,
                                      std::string_view            template_name);

// Decode. On error returns the enum value; on success fills `out` with
// the parsed state and sets `template_name_out`.
[[nodiscard]] GwMatError decode_gwmat(std::span<const std::byte> blob,
                                        MaterialInstanceState&     out,
                                        std::string&               template_name_out);

// Compute the .gwmat content hash for the given payload bytes. Used by
// the codec internally and by tests to verify determinism.
[[nodiscard]] std::uint64_t gwmat_content_hash(std::span<const std::byte> header_bytes,
                                                 std::span<const std::byte> payload_bytes) noexcept;

// Convenience — examine the header without decoding the full payload.
[[nodiscard]] GwMatError peek_gwmat(std::span<const std::byte> blob,
                                      GwMatHeader& header_out) noexcept;

} // namespace gw::render::material
