// tests/fuzz/fuzz_dialogue_graph.cpp — narrative line harness.

#include "engine/narrative/dialogue_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size) {
    if (data == nullptr) {
        return 0;
    }
    gw::narrative::parse_dialogue_graph_fuzz_harness(std::span<const std::uint8_t>(data, size));
    return 0;
}
