// tests/fuzz/fuzz_dialogue_graph.cpp — Part C §25 scaffold (ADR-0117).
// LibFuzzer harness for the narrative dialogue graph parser.

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // TODO(#gw-fuzz-25): parse via gw::narrative::parse_dialogue_graph().
    (void)data;
    (void)size;
    return 0;
}
