// tests/fuzz/fuzz_mod_manifest.cpp — `mod_manifest.json` structural parse (no disk).

#include "engine/scripting/mod_api.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size) {
    if (data == nullptr) {
        return 0;
    }
    const std::string_view sv{reinterpret_cast<const char*>(data), size};
    gw::scripting::ModManifest man{};
    (void)gw::scripting::try_parse_mod_manifest_from_json(sv, man);
    return 0;
}
