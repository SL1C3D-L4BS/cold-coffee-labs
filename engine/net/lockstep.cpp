// engine/net/lockstep.cpp — Phase 14 Wave 14H (ADR-0054).

#include "engine/net/lockstep.hpp"

#include <algorithm>

namespace gw::net {

LockstepSession::LockstepSession(LockstepConfig cfg, LockstepCallbacks cbs)
    : cfg_{cfg}, cbs_{std::move(cbs)} {
    if (cfg_.max_rollback == 0) cfg_.max_rollback = 1;
    ring_.resize(cfg_.max_rollback);
}

void LockstepSession::add_local_input(PeerId local, std::span<const std::uint8_t> input) {
    InputSlot s{};
    s.peer = local;
    s.frame = frame_ + cfg_.frame_delay;
    s.bytes.assign(input.begin(), input.end());
    pending_.push_back(std::move(s));
}

void LockstepSession::add_remote_input(PeerId peer, Tick frame, std::span<const std::uint8_t> input) {
    // Dedup: if we already have a matching (peer, frame) slot, overwrite
    // with the authoritative late-arrival; trigger rollback if the bytes
    // differ from what we speculated on.
    for (auto& slot : pending_) {
        if (slot.peer == peer && slot.frame == frame) {
            const bool differ = (slot.bytes.size() != input.size()) ||
                                !std::equal(slot.bytes.begin(), slot.bytes.end(), input.begin());
            slot.bytes.assign(input.begin(), input.end());
            if (differ && frame < frame_) {
                // Roll back: load the snapshot from `frame`, re-advance.
                for (std::uint32_t i = 0; i < size_; ++i) {
                    const std::uint32_t idx = (head_ + cfg_.max_rollback - 1 - i) % cfg_.max_rollback;
                    if (ring_[idx].frame == frame && cbs_.load_frame) {
                        (void)cbs_.load_frame(frame, std::span<const std::uint8_t>(ring_[idx].snapshot));
                        // Replay through the ring to the live frame.
                        const Tick orig_frame = frame_;
                        frame_ = frame;
                        while (frame_ < orig_frame) {
                            if (cbs_.advance_frame) cbs_.advance_frame(collect_inputs_(frame_));
                            ++frame_;
                        }
                        rollback_count_ += 1;
                        break;
                    }
                }
            }
            return;
        }
    }
    InputSlot s{};
    s.peer = peer;
    s.frame = frame;
    s.bytes.assign(input.begin(), input.end());
    pending_.push_back(std::move(s));
}

InputSet LockstepSession::collect_inputs_(Tick frame) {
    InputSet set{};
    set.frame = frame;
    set.inputs.reserve(cfg_.peer_count);
    for (const auto& slot : pending_) {
        if (slot.frame == frame) set.inputs.push_back(slot);
    }
    return set;
}

bool LockstepSession::tick() {
    // Snapshot the current frame before advancing.
    if (cbs_.save_frame) {
        FrameRec rec{};
        rec.frame    = frame_;
        rec.snapshot = cbs_.save_frame(frame_);
        rec.inputs   = collect_inputs_(frame_);
        ring_[head_] = std::move(rec);
        head_ = (head_ + 1) % cfg_.max_rollback;
        if (size_ < cfg_.max_rollback) ++size_;
    }
    if (cbs_.advance_frame) cbs_.advance_frame(collect_inputs_(frame_));
    if (cbs_.frame_hash) hash_ = cbs_.frame_hash(frame_);
    ++frame_;
    // GC pending inputs older than current frame.
    pending_.erase(std::remove_if(pending_.begin(), pending_.end(),
        [this](const InputSlot& s) { return s.frame + cfg_.max_rollback < frame_; }),
        pending_.end());
    return true;
}

bool LockstepSession::run_sync_test(std::uint32_t frames) {
    if (!cbs_.save_frame || !cbs_.load_frame || !cbs_.advance_frame || !cbs_.frame_hash) return false;
    const Tick start = frame_;
    const auto snap_start = cbs_.save_frame(start);
    const std::uint64_t h0 = cbs_.frame_hash(start);
    for (std::uint32_t i = 0; i < frames; ++i) (void)tick();
    const std::uint64_t h1 = cbs_.frame_hash(frame_);
    // Roll back.
    if (!cbs_.load_frame(start, std::span<const std::uint8_t>(snap_start))) return false;
    frame_ = start;
    if (cbs_.frame_hash(start) != h0) return false;
    for (std::uint32_t i = 0; i < frames; ++i) (void)tick();
    return cbs_.frame_hash(frame_) == h1;
}

} // namespace gw::net
