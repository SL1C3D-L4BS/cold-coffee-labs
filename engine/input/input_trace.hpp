#pragma once
// engine/input/input_trace.hpp — .input_trace serializer/deserializer.
//
// Binary layout per ADR-0020 §2.9:
//   "INTR" u32 version=1
//   u64 start_ns
//   repeated: u64 ns_since_start, u16 kind, u16 payload_len, bytes payload

#include "engine/input/input_types.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace gw::input::trace {

constexpr uint32_t kVersion = 1;
constexpr char kMagic[4] = {'I', 'N', 'T', 'R'};

[[nodiscard]] std::vector<uint8_t> encode(std::span<const RawEvent> events, uint64_t start_ns = 0);
[[nodiscard]] bool decode(std::span<const uint8_t> bytes, std::vector<RawEvent>& out);

} // namespace gw::input::trace
