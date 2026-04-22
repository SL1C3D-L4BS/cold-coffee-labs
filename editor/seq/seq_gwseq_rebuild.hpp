#pragma once
// editor/seq/seq_gwseq_rebuild.hpp
// Non-allocating hot path stays in gw_seq; this editor helper re-encodes an
// entire .gwseq buffer when BLD or the Sequencer panel mutates track tables.

#include "engine/scene/seq/gwseq_codec.hpp"

#include <cstdint>
#include <vector>

namespace gw::editor::seq {

/// Re-encodes `bytes` as a fresh .gwseq containing every track in `reader` plus
/// a new **empty** transform position track (no keyframes yet) with `new_track_id`.
/// Fails on duplicate `new_track_id` or `SeqError` from the codec.
[[nodiscard]] gw::seq::SeqResult append_empty_position_transform_track(
    gw::seq::GwseqReader& reader, std::vector<std::uint8_t>& bytes,
    std::uint32_t new_track_id) noexcept;

}  // namespace gw::editor::seq
