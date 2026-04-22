#pragma once

#include "engine/persist/gwsave_format.hpp"
#include "engine/persist/persist_types.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace gw::persist {

[[nodiscard]] std::vector<std::byte> write_gwsave_container(std::uint16_t                        flags,
                                                            const gwsave::HeaderPrefix&          hdr,
                                                            const std::vector<gwsave::ChunkPayload>& chunks);

struct GwsavePeek {
    LoadError                     err{LoadError::Ok};
    gwsave::HeaderPrefix          hdr{};
    std::vector<gwsave::TocEntry> toc{};
    std::uint64_t                 body_end{0}; // byte offset where footer starts
};

[[nodiscard]] GwsavePeek peek_gwsave(std::span<const std::byte> file, bool verify_integrity);

/// Loads one chunk by TOC index. `file` must be the full container.
[[nodiscard]] LoadError load_gwsave_chunk_by_index(std::span<const std::byte> file,
                                                   const GwsavePeek&          peek,
                                                   std::uint32_t              chunk_index,
                                                   std::vector<std::byte>&    out_payload);

/// Loads all chunk payloads (order matches TOC).
[[nodiscard]] LoadError load_gwsave_all_chunks(std::span<const std::byte> file,
                                               const GwsavePeek&          peek,
                                               std::vector<gwsave::ChunkPayload>& out_chunks);

} // namespace gw::persist
