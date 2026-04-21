#pragma once
// engine/net/lag_comp.hpp — Phase 14 Wave 14E (ADR-0050).
//
// Server-side lag compensation: ring of archived frames containing just
// enough world state to re-evaluate hit queries at a past tick without
// actually rolling the live simulation back. Each frame is fingerprinted
// with the determinism hash so the engine can detect archive corruption.

#include "engine/net/net_types.hpp"

#include <cstdint>
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace gw::net {

struct ArchivedTransform {
    EntityReplicationId id{};
    glm::dvec3          pos_ws{};
    glm::quat           rot{1.0f, 0.0f, 0.0f, 0.0f};
};

struct ArchivedFrame {
    Tick                            tick{0};
    std::uint64_t                   determinism_hash{0};
    std::vector<ArchivedTransform>  transforms{};
};

// ServerRewind — fixed-capacity ring of archived frames. All allocations
// happen at construction; push-oldest eviction is O(1). Threading: single
// producer (server sim thread) + readers on job threads via `read_frame`.
class ServerRewind {
public:
    explicit ServerRewind(std::uint32_t capacity_frames = 12) { set_capacity(capacity_frames); }

    void set_capacity(std::uint32_t cap) {
        capacity_ = (cap == 0) ? 1u : cap;
        ring_.resize(capacity_);
        head_ = 0;
        size_ = 0;
    }

    void archive_frame(const ArchivedFrame& f) {
        ring_[head_] = f;
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) ++size_;
    }

    // Locate the archived frame at/immediately before `target`. Returns
    // nullptr if target is older than the oldest retained frame.
    [[nodiscard]] const ArchivedFrame* read_frame(Tick target) const noexcept {
        const ArchivedFrame* best = nullptr;
        for (std::uint32_t i = 0; i < size_; ++i) {
            const auto& f = ring_[i];
            if (f.tick <= target && (!best || f.tick > best->tick)) {
                best = &f;
            }
        }
        return best;
    }

    [[nodiscard]] std::uint64_t archive_hash(Tick target) const noexcept {
        const auto* f = read_frame(target);
        return f ? f->determinism_hash : 0;
    }

    [[nodiscard]] std::uint32_t history_frames() const noexcept { return size_; }
    [[nodiscard]] std::uint32_t capacity()        const noexcept { return capacity_; }
    void clear() noexcept { head_ = 0; size_ = 0; }

private:
    std::vector<ArchivedFrame> ring_{};
    std::uint32_t              capacity_{12};
    std::uint32_t              head_{0};
    std::uint32_t              size_{0};
};

} // namespace gw::net
