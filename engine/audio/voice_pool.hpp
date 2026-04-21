#pragma once
// engine/audio/voice_pool.hpp — fixed-capacity voice allocator (Wave 10A).
//
// Two pools: 2D (non-spatial) and 3D (spatial). Each is a free-list backed
// by a flat array sized at construction. Generation counters invalidate
// stale handles so a steal-oldest-on-saturation policy never hands out
// dangling references.
//
// Hot-path guarantees (see ADR-0017 §2.3):
//   - no heap allocation after construction
//   - acquire / release are O(1)
//   - handle comparison is 32-bit equality
//
// The pool is accessed from both the main thread (play/stop) and the audio
// thread (render). The current Phase-10 implementation serialises access via
// an intrusive free-list + sequence counter — cheap, contention-free because
// play/stop events are quoted onto a SPSC command queue consumed by the
// audio thread between render buffers (see AudioService). The pool itself
// does not need atomics; the command queue provides the ordering.

#include "engine/audio/audio_types.hpp"

#include <array>
#include <cstddef>
#include <memory>

namespace gw::audio {

struct VoiceSlotInfo {
    VoiceHandle  handle{};    // null when free
    VoiceClass   klass{VoiceClass::NonSpatial2D};
    BusId        bus{BusId::Master};
    AudioClipId  clip{};
    EmitterState emitter{};   // 3D only
    Play2D       play2d{};    // 2D only
    Play3D       play3d{};    // 3D only
    uint64_t     started_frame{0};
    bool         active{false};
    bool         looping{false};
};

class VoicePool {
public:
    struct Config {
        uint16_t capacity_2d{64};
        uint16_t capacity_3d{32};
    };

    explicit VoicePool(Config cfg);
    ~VoicePool() = default;

    VoicePool(const VoicePool&) = delete;
    VoicePool& operator=(const VoicePool&) = delete;

    // Acquire a slot. On saturation returns the oldest-started live voice
    // (steal-oldest) and increments a counter in `stolen_count_` so the
    // service can surface it in stats. Generation counters make any old
    // handle to the stolen slot compare unequal.
    [[nodiscard]] VoiceHandle acquire_2d(BusId bus, AudioClipId clip, const Play2D& p, uint64_t now_frame);
    [[nodiscard]] VoiceHandle acquire_3d(AudioClipId clip, const Play3D& p, uint64_t now_frame);

    void release(VoiceHandle h) noexcept;

    [[nodiscard]] bool is_live(VoiceHandle h) const noexcept;

    [[nodiscard]] VoiceSlotInfo*       get_mut(VoiceHandle h) noexcept;
    [[nodiscard]] const VoiceSlotInfo* get(VoiceHandle h) const noexcept;

    [[nodiscard]] uint32_t active_2d() const noexcept { return active_2d_; }
    [[nodiscard]] uint32_t active_3d() const noexcept { return active_3d_; }
    [[nodiscard]] uint32_t stolen_count() const noexcept { return stolen_count_; }
    [[nodiscard]] uint16_t capacity_2d() const noexcept { return cap_2d_; }
    [[nodiscard]] uint16_t capacity_3d() const noexcept { return cap_3d_; }

    // Iterate all active slots (for mixing / debug). Read-only.
    template <typename F>
    void for_each_active(F&& fn) const {
        for (uint32_t i = 0; i < cap_2d_; ++i) {
            const auto& s = slots_2d_[i];
            if (s.active) fn(s);
        }
        for (uint32_t i = 0; i < cap_3d_; ++i) {
            const auto& s = slots_3d_[i];
            if (s.active) fn(s);
        }
    }

    template <typename F>
    void for_each_active_mut(F&& fn) {
        for (uint32_t i = 0; i < cap_2d_; ++i) {
            auto& s = slots_2d_[i];
            if (s.active) fn(s);
        }
        for (uint32_t i = 0; i < cap_3d_; ++i) {
            auto& s = slots_3d_[i];
            if (s.active) fn(s);
        }
    }

private:
    // Per-class storage. `slots` is flat; indices 0..cap-1 are valid.
    // `free_head_` is a singly linked free-list of free indices, terminated
    // by kInvalidIndex.
    static constexpr uint16_t kInvalidIndex = 0xFFFF;

    struct FreeNode {
        uint16_t next{kInvalidIndex};
    };

    uint16_t cap_2d_{0};
    uint16_t cap_3d_{0};

    // We store slots + free-list side-tables in owning unique_ptr arrays to
    // obey CLAUDE.md #5 (no raw new/delete). The pool never grows, so
    // pointers into these arrays are stable for the pool's lifetime.
    struct Storage {
        std::unique_ptr<VoiceSlotInfo[]> slots;
        std::unique_ptr<uint16_t[]>      gens;
        std::unique_ptr<uint16_t[]>      free_next;
    };
    Storage storage_2d_{};
    Storage storage_3d_{};

    // Raw aliases for hot-path speed (owned by `storage_*_`).
    VoiceSlotInfo* slots_2d_{nullptr};
    VoiceSlotInfo* slots_3d_{nullptr};
    uint16_t*      gens_2d_{nullptr};
    uint16_t*      gens_3d_{nullptr};
    uint16_t*      free_next_2d_{nullptr};
    uint16_t*      free_next_3d_{nullptr};

    uint16_t free_head_2d_{kInvalidIndex};
    uint16_t free_head_3d_{kInvalidIndex};

    uint32_t active_2d_{0};
    uint32_t active_3d_{0};
    uint32_t stolen_count_{0};

    void init_class(uint16_t cap, Storage& storage, uint16_t& free_head);
    [[nodiscard]] VoiceHandle acquire_slot(VoiceClass klass, uint64_t now_frame);
    [[nodiscard]] uint16_t steal_oldest(VoiceClass klass) noexcept;
};

} // namespace gw::audio
