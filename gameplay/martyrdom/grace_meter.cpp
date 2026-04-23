// gameplay/martyrdom/grace_meter.cpp — Phase 22 scaffold.

#include "gameplay/martyrdom/grace_meter.hpp"

namespace gw::gameplay::martyrdom {

namespace {

std::uint64_t hash_tx(const narrative::GraceTransaction& tx) noexcept {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    h = (h ^ static_cast<std::uint64_t>(tx.actor_id)) * 0x100000001b3ULL;
    h = (h ^ static_cast<std::uint64_t>(tx.source))   * 0x100000001b3ULL;
    h = (h ^ tx.seed)                                 * 0x100000001b3ULL;
    return h;
}

} // namespace

bool try_apply_grace(narrative::GraceComponent&     grace,
                    GraceBookkeeping&               book,
                    const narrative::GraceTransaction& tx) noexcept {
    const std::uint64_t h = hash_tx(tx);
    for (std::uint16_t i = 0; i < book.seen_count; ++i) {
        if (book.seen_tx_hash[i] == h) return false;
    }
    if (book.seen_count < GraceBookkeeping::MAX_SEEN) {
        book.seen_tx_hash[book.seen_count++] = h;
    }
    narrative::apply_grace_transaction(grace, tx);
    return true;
}

} // namespace gw::gameplay::martyrdom
