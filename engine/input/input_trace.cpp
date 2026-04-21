// engine/input/input_trace.cpp
#include "engine/input/input_trace.hpp"

#include <cstring>

namespace gw::input::trace {

namespace {

inline void store_u16_le(uint8_t* p, uint16_t v) noexcept {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
}
inline void store_u32_le(uint8_t* p, uint32_t v) noexcept {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}
inline void store_u64_le(uint8_t* p, uint64_t v) noexcept {
    for (int i = 0; i < 8; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}
inline uint16_t load_u16_le(const uint8_t* p) noexcept {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
inline uint32_t load_u32_le(const uint8_t* p) noexcept {
    return static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}
inline uint64_t load_u64_le(const uint8_t* p) noexcept {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(p[i]) << (8 * i);
    return v;
}

// RawEvent payload is a fixed 40 bytes on the wire:
//   u64 device.value
//   u32 code
//   f32 fval[0..2]
//   i16 ival[0..1]
//   u16 modifiers
constexpr std::size_t kPayloadBytes = 8 + 4 + 4 * 3 + 2 * 2 + 2;  // 30
// Round up to 32 bytes with 2 bytes padding for cacheline happiness.
constexpr std::size_t kPayloadOnWire = 32;

void pack_payload(const RawEvent& ev, uint8_t* p) noexcept {
    store_u64_le(p, ev.device.value);
    store_u32_le(p + 8, ev.code);
    std::memcpy(p + 12, &ev.fval[0], 4);
    std::memcpy(p + 16, &ev.fval[1], 4);
    std::memcpy(p + 20, &ev.fval[2], 4);
    store_u16_le(p + 24, static_cast<uint16_t>(ev.ival[0]));
    store_u16_le(p + 26, static_cast<uint16_t>(ev.ival[1]));
    store_u16_le(p + 28, ev.modifiers);
    p[30] = 0; p[31] = 0;
}

void unpack_payload(const uint8_t* p, RawEvent& ev) noexcept {
    ev.device.value = load_u64_le(p);
    ev.code = load_u32_le(p + 8);
    std::memcpy(&ev.fval[0], p + 12, 4);
    std::memcpy(&ev.fval[1], p + 16, 4);
    std::memcpy(&ev.fval[2], p + 20, 4);
    ev.ival[0] = static_cast<int16_t>(load_u16_le(p + 24));
    ev.ival[1] = static_cast<int16_t>(load_u16_le(p + 26));
    ev.modifiers = load_u16_le(p + 28);
}

} // namespace

std::vector<uint8_t> encode(std::span<const RawEvent> events, uint64_t start_ns) {
    std::vector<uint8_t> out;
    out.resize(4 + 4 + 8);   // magic + version + start_ns
    std::memcpy(out.data(), kMagic, 4);
    store_u32_le(out.data() + 4, kVersion);
    store_u64_le(out.data() + 8, start_ns);

    const std::size_t record_bytes = 8 + 2 + 2 + kPayloadOnWire;  // ts + kind + len + payload
    out.reserve(out.size() + events.size() * record_bytes);

    for (const auto& ev : events) {
        const std::size_t base = out.size();
        out.resize(base + record_bytes);
        uint8_t* p = out.data() + base;
        store_u64_le(p, ev.timestamp_ns - start_ns);
        store_u16_le(p + 8, static_cast<uint16_t>(ev.kind));
        store_u16_le(p + 10, static_cast<uint16_t>(kPayloadOnWire));
        pack_payload(ev, p + 12);
    }
    return out;
}

bool decode(std::span<const uint8_t> bytes, std::vector<RawEvent>& out) {
    out.clear();
    if (bytes.size() < 16) return false;
    if (bytes[0] != 'I' || bytes[1] != 'N' || bytes[2] != 'T' || bytes[3] != 'R') return false;
    if (load_u32_le(bytes.data() + 4) != kVersion) return false;
    const uint64_t start_ns = load_u64_le(bytes.data() + 8);

    std::size_t off = 16;
    while (off + 12 <= bytes.size()) {
        const uint8_t* p = bytes.data() + off;
        const uint64_t rel_ns = load_u64_le(p);
        const uint16_t kind   = load_u16_le(p + 8);
        const uint16_t len    = load_u16_le(p + 10);
        if (off + 12 + len > bytes.size()) return false;

        RawEvent ev{};
        ev.timestamp_ns = start_ns + rel_ns;
        ev.kind = static_cast<RawEventKind>(kind);
        if (len >= kPayloadOnWire) {
            unpack_payload(p + 12, ev);
        }
        out.push_back(ev);
        off += 12 + len;
    }
    return true;
}

} // namespace gw::input::trace
