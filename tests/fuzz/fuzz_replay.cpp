// tests/fuzz/fuzz_replay.cpp — JSON gameplay-replay shape validation
// (cross-check bld-replay::GameplayReplay::validate semantics for version/ticks).

#include "engine/replay/gameplay_replay_validate.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, const std::size_t size) {
    if (data == nullptr) {
        return 0;
    }
    // Require printable-ish payload for the loose JSON validator; ignore binary noise.
    std::string s;
    s.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        const char c = static_cast<char>(data[i]);
        if (c == '\n' || c == '\r' || c == '\t' || (c >= 0x20 && c < 0x7F)) {
            s.push_back(c);
        }
    }
    (void)gw::replay::validate_gameplay_replay_json_loose(std::string_view{s});
    return 0;
}
