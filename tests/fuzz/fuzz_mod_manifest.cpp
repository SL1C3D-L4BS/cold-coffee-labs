// tests/fuzz/fuzz_mod_manifest.cpp — Part C §25 scaffold (ADR-0117).
// LibFuzzer harness for the mod manifest JSON schema loader.

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // TODO(#gw-fuzz-25): parse via gw::mods::load_manifest_from_bytes().
    (void)data;
    (void)size;
    return 0;
}
