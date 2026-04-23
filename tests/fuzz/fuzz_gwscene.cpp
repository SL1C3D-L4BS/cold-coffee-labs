// tests/fuzz/fuzz_gwscene.cpp — Part C §25 scaffold (ADR-0117).
// LibFuzzer harness for the .gwscene loader.

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // TODO(#gw-fuzz-25): call gw::scene::parse_gwscene(data, size) once the
    // loader exposes a byte-buffer entry point.
    (void)data;
    (void)size;
    return 0;
}
