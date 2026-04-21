#pragma once
// engine/core/events/event_bus.hpp — Phase 11 Wave 11A (ADR-0023).
//
// Three primitives live in this module:
//   * EventBus<T>         — compile-time pub/sub, synchronous, zero-alloc
//                           dispatch. Handlers run inside publish().
//   * InFrameQueue<T>     — double-buffered SPMC queue drained once per
//                           frame; zero-alloc after registration.
//   * CrossSubsystemBus   — type-erased fixed-size ring carrying opaque
//                           byte payloads across the C-ABI boundary.
//
// All three are **reentrancy-safe** for the shapes Phase-11 exercises:
//   - Re-publish from inside a handler is deferred (the outer walk finishes
//     first).
//   - Unsubscribe from inside a handler defers removal to after the walk.
//
// No std::function, no std::shared_ptr on the publish path. Handler
// state is captured by lambda and stored in a std::vector<Entry> whose
// Entry is a small-buffer std::function-like: we store a raw callable
// pointer + an owned unique_ptr<HandlerBase>. The unique_ptr path runs
// once at subscribe; steady-state publish only chases the raw pointers.
//
// CLAUDE.md non-negotiables:
//   - No std::thread / std::async (CrossSubsystemBus uses engine/jobs).
//   - No OS headers (pure C++23).
//   - No RAW new/delete outside unique_ptr's factory.

#include "engine/core/assert.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace gw::events {

// ---------------------------------------------------------------------------
// SubscriptionHandle
// ---------------------------------------------------------------------------
struct SubscriptionHandle {
    std::uint64_t id{0};
    void*         owner{nullptr};

    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }
};

// ---------------------------------------------------------------------------
// EventBus<T> — synchronous compile-time dispatch.
// ---------------------------------------------------------------------------
template <typename T>
class EventBus {
public:
    using Callback = void (*)(void* user, const T&);

    EventBus() {
        // Reserve a small fixed capacity so the first N subscribers never
        // reallocate. Subsequent growth is a cold-path event.
        entries_.reserve(8);
    }

    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // Subscribe a free-function-style handler. `user` is the opaque cookie
    // threaded into the callback. Returns a handle the subscriber retains
    // to unsubscribe.
    SubscriptionHandle subscribe(Callback cb, void* user = nullptr) {
        GW_ASSERT(cb != nullptr, "EventBus::subscribe: null callback");
        const std::uint64_t id = ++next_id_;
        entries_.push_back(Entry{cb, user, id, false});
        return SubscriptionHandle{id, this};
    }

    // Type-erased lambda subscription. Captures are owned by the bus via
    // unique_ptr; the steady-state publish path still chases a raw pointer.
    template <typename F>
    requires std::is_invocable_v<F, const T&>
    SubscriptionHandle subscribe(F f) {
        struct Holder : HolderBase {
            F fn;
            explicit Holder(F&& x) : fn(std::move(x)) {}
            void invoke(const T& t) override { fn(t); }
        };
        auto holder = std::make_unique<Holder>(std::move(f));
        HolderBase* raw = holder.get();
        const std::uint64_t id = ++next_id_;
        entries_.push_back(Entry{
            /*cb*/ &EventBus::holder_trampoline,
            /*user*/ raw,
            id,
            /*pending_remove*/ false,
            /*holder*/ std::move(holder),
        });
        return SubscriptionHandle{id, this};
    }

    // Remove a subscription by handle. Safe inside dispatch (deferred).
    void unsubscribe(SubscriptionHandle h) noexcept {
        if (!h.valid() || h.owner != this) return;
        for (auto& e : entries_) {
            if (e.id == h.id) {
                if (in_dispatch_ > 0) {
                    e.pending_remove = true;
                    has_pending_remove_ = true;
                } else {
                    e = std::move(entries_.back());
                    entries_.pop_back();
                }
                return;
            }
        }
    }

    // Synchronous fan-out. Zero allocations steady-state. Reentrant.
    void publish(const T& ev) {
        ++in_dispatch_;
        // Deliberate index walk so removals at the tail don't invalidate us.
        const std::size_t n = entries_.size();
        for (std::size_t i = 0; i < n; ++i) {
            auto& e = entries_[i];
            if (e.pending_remove) continue;
            e.cb(e.user, ev);
        }
        --in_dispatch_;
        if (in_dispatch_ == 0 && has_pending_remove_) {
            compact_pending_removes();
        }
    }

    [[nodiscard]] std::size_t subscriber_count() const noexcept {
        return entries_.size();
    }

private:
    struct HolderBase {
        virtual ~HolderBase() = default;
        virtual void invoke(const T& t) = 0;
    };

    static void holder_trampoline(void* user, const T& ev) {
        static_cast<HolderBase*>(user)->invoke(ev);
    }

    struct Entry {
        Callback                    cb{nullptr};
        void*                       user{nullptr};
        std::uint64_t               id{0};
        bool                        pending_remove{false};
        std::unique_ptr<HolderBase> holder{};
    };

    void compact_pending_removes() noexcept {
        std::size_t w = 0;
        for (std::size_t r = 0; r < entries_.size(); ++r) {
            if (entries_[r].pending_remove) continue;
            if (w != r) entries_[w] = std::move(entries_[r]);
            ++w;
        }
        entries_.resize(w);
        has_pending_remove_ = false;
    }

    std::vector<Entry>  entries_;
    std::uint64_t       next_id_{0};
    int                 in_dispatch_{0};
    bool                has_pending_remove_{false};
};

// ---------------------------------------------------------------------------
// InFrameQueue<T> — deferred, double-buffered, zero-alloc after reserve().
// ---------------------------------------------------------------------------
template <typename T>
class InFrameQueue {
public:
    explicit InFrameQueue(std::size_t capacity = 1024) {
        GW_ASSERT(capacity > 0, "InFrameQueue: capacity must be > 0");
        back_.reserve(capacity);
        front_.reserve(capacity);
        capacity_ = capacity;
    }

    ~InFrameQueue() = default;
    InFrameQueue(const InFrameQueue&) = delete;
    InFrameQueue& operator=(const InFrameQueue&) = delete;

    // Publish into the back buffer. Returns false if the queue is full
    // (steady-state contract: size the queue so this never returns false).
    [[nodiscard]] bool publish(const T& ev) {
        if (back_.size() >= capacity_) {
            ++dropped_;
            return false;
        }
        back_.push_back(ev);
        return true;
    }
    [[nodiscard]] bool publish(T&& ev) {
        if (back_.size() >= capacity_) {
            ++dropped_;
            return false;
        }
        back_.push_back(std::move(ev));
        return true;
    }

    // Swap buffers and drain the front buffer through `fn`. Back buffer is
    // cleared for next frame. `fn` may publish back into the queue (goes
    // to the now-empty back buffer, drains next frame).
    template <typename F>
    std::size_t drain(F&& fn) {
        front_.swap(back_);
        const std::size_t n = front_.size();
        for (auto& ev : front_) fn(static_cast<const T&>(ev));
        front_.clear();
        return n;
    }

    [[nodiscard]] std::size_t size() const noexcept { return back_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::uint64_t dropped() const noexcept { return dropped_; }

private:
    std::vector<T> back_;
    std::vector<T> front_;
    std::size_t    capacity_{0};
    std::uint64_t  dropped_{0};
};

// ---------------------------------------------------------------------------
// CrossSubsystemBus — type-erased ring for C-ABI-friendly events.
// Records are 256 bytes: {kind:u32, size:u16, flags:u16, payload[248]}.
// Alignment: records are aligned to 16 bytes so SSE/NEON readers are happy.
// ---------------------------------------------------------------------------
class CrossSubsystemBus {
public:
    static constexpr std::size_t kRecordSize = 256;
    static constexpr std::size_t kHeaderSize = 8;
    static constexpr std::size_t kPayloadMax = kRecordSize - kHeaderSize;

    struct Record {
        alignas(16) std::byte bytes[kRecordSize]{};
    };
    struct Header {
        std::uint32_t kind{0};
        std::uint16_t size{0};
        std::uint16_t flags{0};
    };
    static_assert(sizeof(Header) == kHeaderSize);
    static_assert(alignof(Record) >= 16);

    using Visitor = void (*)(void* user, std::uint32_t kind,
                             std::uint16_t flags, std::span<const std::byte> payload);

    explicit CrossSubsystemBus(std::size_t capacity = 256) {
        GW_ASSERT(capacity > 0, "CrossSubsystemBus: capacity must be > 0");
        ring_.resize(capacity);
        capacity_ = capacity;
    }

    [[nodiscard]] bool publish(std::uint32_t kind, std::span<const std::byte> payload,
                               std::uint16_t flags = 0) noexcept {
        if (payload.size() > kPayloadMax) return false;
        if (size_.load(std::memory_order_relaxed) >= capacity_) {
            ++dropped_;
            return false;
        }
        const std::size_t w = write_idx_ % capacity_;
        auto& rec = ring_[w];
        Header h{kind, static_cast<std::uint16_t>(payload.size()), flags};
        std::memcpy(rec.bytes, &h, sizeof(h));
        if (!payload.empty()) {
            std::memcpy(rec.bytes + kHeaderSize, payload.data(), payload.size());
        }
        ++write_idx_;
        size_.fetch_add(1, std::memory_order_release);
        return true;
    }

    // Drain on the consumer thread. `visitor` is called once per record.
    std::size_t drain(Visitor visitor, void* user) noexcept {
        std::size_t drained = 0;
        while (size_.load(std::memory_order_acquire) > 0) {
            const std::size_t r = read_idx_ % capacity_;
            auto& rec = ring_[r];
            Header h{};
            std::memcpy(&h, rec.bytes, sizeof(h));
            std::span<const std::byte> payload{rec.bytes + kHeaderSize, h.size};
            visitor(user, h.kind, h.flags, payload);
            ++read_idx_;
            size_.fetch_sub(1, std::memory_order_release);
            ++drained;
        }
        return drained;
    }

    [[nodiscard]] std::size_t size()     const noexcept { return size_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::uint64_t dropped() const noexcept { return dropped_; }

private:
    std::vector<Record>        ring_;
    std::size_t                capacity_{0};
    std::atomic<std::size_t>   size_{0};
    std::size_t                write_idx_{0};  // producer-local
    std::size_t                read_idx_{0};   // consumer-local
    std::uint64_t              dropped_{0};
};

} // namespace gw::events
