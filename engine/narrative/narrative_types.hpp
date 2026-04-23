#pragma once

#include <cstdint>

namespace gw::narrative {

enum class Act : std::uint8_t {
    None = 0,
    I    = 1,
    II   = 2,
    III  = 3,
};

enum class Speaker : std::uint8_t {
    None     = 0,
    Malakor  = 1,
    Niccolo  = 2,
    Witness  = 3,
};

struct DialogueLineId {
    std::uint32_t value = 0;
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    friend constexpr bool operator==(DialogueLineId, DialogueLineId) noexcept = default;
};

} // namespace gw::narrative
