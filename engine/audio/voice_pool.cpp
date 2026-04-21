// engine/audio/voice_pool.cpp — see header for contract.
#include "engine/audio/voice_pool.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace gw::audio {

VoicePool::VoicePool(Config cfg) : cap_2d_(cfg.capacity_2d), cap_3d_(cfg.capacity_3d) {
    init_class(cap_2d_, storage_2d_, free_head_2d_);
    init_class(cap_3d_, storage_3d_, free_head_3d_);

    slots_2d_     = storage_2d_.slots.get();
    slots_3d_     = storage_3d_.slots.get();
    gens_2d_      = storage_2d_.gens.get();
    gens_3d_      = storage_3d_.gens.get();
    free_next_2d_ = storage_2d_.free_next.get();
    free_next_3d_ = storage_3d_.free_next.get();
}

void VoicePool::init_class(uint16_t cap, Storage& storage, uint16_t& free_head) {
    if (cap == 0) {
        free_head = kInvalidIndex;
        return;
    }
    storage.slots     = std::make_unique<VoiceSlotInfo[]>(cap);
    storage.gens      = std::make_unique<uint16_t[]>(cap);
    storage.free_next = std::make_unique<uint16_t[]>(cap);

    // Build the free-list: 0 -> 1 -> 2 -> ... -> cap-1 -> invalid.
    for (uint16_t i = 0; i < cap; ++i) {
        storage.gens[i] = 1;  // generation starts at 1 so {0,0} is always the null handle.
        storage.free_next[i] = (i + 1 == cap) ? kInvalidIndex : static_cast<uint16_t>(i + 1);
    }
    free_head = 0;
}

VoiceHandle VoicePool::acquire_slot(VoiceClass klass, uint64_t now_frame) {
    uint16_t* free_head  = (klass == VoiceClass::NonSpatial2D) ? &free_head_2d_ : &free_head_3d_;
    uint16_t* free_next  = (klass == VoiceClass::NonSpatial2D) ? free_next_2d_ : free_next_3d_;
    uint16_t* gens       = (klass == VoiceClass::NonSpatial2D) ? gens_2d_       : gens_3d_;
    VoiceSlotInfo* slots = (klass == VoiceClass::NonSpatial2D) ? slots_2d_      : slots_3d_;
    uint32_t& active     = (klass == VoiceClass::NonSpatial2D) ? active_2d_     : active_3d_;
    uint16_t cap         = (klass == VoiceClass::NonSpatial2D) ? cap_2d_        : cap_3d_;

    uint16_t idx;
    if (*free_head != kInvalidIndex) {
        idx = *free_head;
        *free_head = free_next[idx];
    } else {
        // Saturated — steal the oldest-started voice in this class.
        idx = steal_oldest(klass);
        if (idx == kInvalidIndex) {
            return VoiceHandle::null();
        }
        // Bump generation so any outstanding VoiceHandle to this slot becomes
        // invalid (ADR-0017 §2.3: handles are generation-versioned so the
        // steal-oldest policy never hands out dangling references).
        gens[idx] = static_cast<uint16_t>(gens[idx] + 1);
        if (gens[idx] == 0) gens[idx] = 1;
        ++stolen_count_;
        --active;  // will re-increment below
    }
    if (idx >= cap) return VoiceHandle::null();

    auto& slot = slots[idx];
    slot = VoiceSlotInfo{};
    slot.active = true;
    slot.klass = klass;
    slot.started_frame = now_frame;
    slot.handle = VoiceHandle{idx, gens[idx]};
    ++active;
    return slot.handle;
}

uint16_t VoicePool::steal_oldest(VoiceClass klass) noexcept {
    VoiceSlotInfo* slots = (klass == VoiceClass::NonSpatial2D) ? slots_2d_ : slots_3d_;
    uint16_t cap         = (klass == VoiceClass::NonSpatial2D) ? cap_2d_   : cap_3d_;

    uint16_t best = kInvalidIndex;
    uint64_t best_frame = UINT64_MAX;
    for (uint16_t i = 0; i < cap; ++i) {
        if (slots[i].active && slots[i].started_frame < best_frame) {
            best_frame = slots[i].started_frame;
            best = i;
        }
    }
    return best;
}

VoiceHandle VoicePool::acquire_2d(BusId bus, AudioClipId clip, const Play2D& p, uint64_t now_frame) {
    auto h = acquire_slot(VoiceClass::NonSpatial2D, now_frame);
    if (h.is_null()) return h;
    auto& slot = slots_2d_[h.index];
    slot.bus = bus;
    slot.clip = clip;
    slot.play2d = p;
    slot.looping = p.looping;
    return h;
}

VoiceHandle VoicePool::acquire_3d(AudioClipId clip, const Play3D& p, uint64_t now_frame) {
    auto h = acquire_slot(VoiceClass::Spatial3D, now_frame);
    if (h.is_null()) return h;
    auto& slot = slots_3d_[h.index];
    slot.bus = p.emitter.bus;
    slot.clip = clip;
    slot.emitter = p.emitter;
    slot.play3d = p;
    slot.looping = p.looping;
    return h;
}

void VoicePool::release(VoiceHandle h) noexcept {
    if (h.is_null()) return;

    // Try 2D then 3D.
    auto try_release = [&](VoiceSlotInfo* slots, uint16_t* gens, uint16_t* free_next,
                           uint16_t& free_head, uint32_t& active, uint16_t cap) -> bool {
        if (h.index >= cap) return false;
        auto& slot = slots[h.index];
        if (!slot.active) return false;
        if (slot.handle != h) return false;  // generation mismatch
        slot.active = false;
        slot.handle = VoiceHandle::null();
        // Bump generation so stale handles are invalid.
        gens[h.index] = static_cast<uint16_t>(gens[h.index] + 1);
        if (gens[h.index] == 0) gens[h.index] = 1;  // never use generation 0
        // Push onto free list.
        free_next[h.index] = free_head;
        free_head = h.index;
        if (active > 0) --active;
        return true;
    };

    if (try_release(slots_2d_, gens_2d_, free_next_2d_, free_head_2d_, active_2d_, cap_2d_)) return;
    (void) try_release(slots_3d_, gens_3d_, free_next_3d_, free_head_3d_, active_3d_, cap_3d_);
}

bool VoicePool::is_live(VoiceHandle h) const noexcept {
    if (h.is_null()) return false;
    if (h.index < cap_2d_ && slots_2d_[h.index].handle == h && slots_2d_[h.index].active) return true;
    if (h.index < cap_3d_ && slots_3d_[h.index].handle == h && slots_3d_[h.index].active) return true;
    return false;
}

VoiceSlotInfo* VoicePool::get_mut(VoiceHandle h) noexcept {
    if (h.index < cap_2d_ && slots_2d_[h.index].handle == h && slots_2d_[h.index].active) return &slots_2d_[h.index];
    if (h.index < cap_3d_ && slots_3d_[h.index].handle == h && slots_3d_[h.index].active) return &slots_3d_[h.index];
    return nullptr;
}

const VoiceSlotInfo* VoicePool::get(VoiceHandle h) const noexcept {
    if (h.index < cap_2d_ && slots_2d_[h.index].handle == h && slots_2d_[h.index].active) return &slots_2d_[h.index];
    if (h.index < cap_3d_ && slots_3d_[h.index].handle == h && slots_3d_[h.index].active) return &slots_3d_[h.index];
    return nullptr;
}

} // namespace gw::audio
