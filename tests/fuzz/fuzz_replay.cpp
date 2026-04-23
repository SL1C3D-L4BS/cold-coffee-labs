// tests/fuzz/fuzz_replay.cpp — Part C §25 scaffold (ADR-0117).
// LibFuzzer harness for the gameplay replay schema (bld-replay/gameplay_schema).

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // TODO(#gw-fuzz-25): bridge into bld-replay via C-ABI once exposed.
    (void)data;
    (void)size;
    return 0;
}
