#pragma once

#include "engine/narrative/grace_meter.hpp"

#include <cstdint>

namespace gw::gameplay::martyrdom {

/// Gameplay-side facade over engine/narrative/grace_meter. Provides the
/// bookkeeping rules docs/07 §2.7 specifies (dedupe by actor+source, etc.).
struct GraceBookkeeping {
    // A small fixed-size dedupe set; real sizing lives in ECS storage.
    static constexpr std::uint16_t MAX_SEEN = 256;
    std::uint64_t seen_tx_hash[MAX_SEEN]{};
    std::uint16_t seen_count = 0;
};

/// Returns true if the transaction was applied (first time seen) or false if
/// the `(actor, source, seed)` tuple was already credited.
bool try_apply_grace(narrative::GraceComponent&     grace,
                    GraceBookkeeping&               book,
                    const narrative::GraceTransaction& tx) noexcept;

} // namespace gw::gameplay::martyrdom
