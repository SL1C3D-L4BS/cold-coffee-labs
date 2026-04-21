#pragma once
// engine/core/events/event_trace.hpp — optional .event_trace sink
// (ADR-0023 §EventTraceSink). Compile with GW_EVENT_TRACE=1 to enable;
// the default is OFF so shipping builds do not pay the file cost.
//
// The binary shape matches Phase 10's `.input_trace`: magic + version,
// then a sequence of {monotonic_ns : u64, kind : u32, size : u16, flags : u16,
// payload[size]} records. Tooling lives under `tools/trace/`.

#include "engine/core/events/event_bus.hpp"
#include "engine/core/events/events_core.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>

namespace gw::events {

class EventTraceSink {
public:
    EventTraceSink() = default;

    [[nodiscard]] bool open(const std::string& path) {
        f_ = std::fopen(path.c_str(), "wb");
        if (!f_) return false;
        constexpr char kMagic[] = "EVTTRACE";
        std::fwrite(kMagic, 1, 8, f_);
        const std::uint32_t version = 1;
        std::fwrite(&version, sizeof(version), 1, f_);
        open_ = true;
        return true;
    }

    void close() noexcept {
        if (f_) { std::fclose(f_); f_ = nullptr; }
        open_ = false;
    }

    ~EventTraceSink() { close(); }
    EventTraceSink(const EventTraceSink&) = delete;
    EventTraceSink& operator=(const EventTraceSink&) = delete;

    // Record an arbitrary event. Caller serialises into bytes.
    void record(std::uint64_t monotonic_ns, std::uint32_t kind,
                std::uint16_t flags, std::span<const std::byte> payload) noexcept {
        if (!open_) return;
        std::uint16_t sz = static_cast<std::uint16_t>(payload.size());
        std::fwrite(&monotonic_ns, sizeof(monotonic_ns), 1, f_);
        std::fwrite(&kind, sizeof(kind), 1, f_);
        std::fwrite(&sz,   sizeof(sz),   1, f_);
        std::fwrite(&flags, sizeof(flags), 1, f_);
        if (sz) std::fwrite(payload.data(), 1, sz, f_);
    }

    template <typename T>
    void record_typed(std::uint64_t monotonic_ns, EventKind kind, const T& ev,
                      std::uint16_t flags = 0) noexcept {
        record(monotonic_ns, static_cast<std::uint32_t>(kind), flags,
               std::span<const std::byte>{reinterpret_cast<const std::byte*>(&ev), sizeof(T)});
    }

    [[nodiscard]] bool is_open() const noexcept { return open_; }

private:
    std::FILE* f_{nullptr};
    bool       open_{false};
};

} // namespace gw::events
