// tests/fuzz/fuzz_bld_ipc.cpp — Part C §25 scaffold (ADR-0117).
// LibFuzzer harness for BLD C-ABI message dispatch.

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // TODO(#gw-fuzz-25): dispatch via editor/bld_api/editor_bld_api.hpp.
    (void)data;
    (void)size;
    return 0;
}
