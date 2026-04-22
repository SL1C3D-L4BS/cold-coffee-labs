#pragma once
// engine/i18n/events_i18n.hpp — ADR-0069 §3.

#include <cstdint>
#include <string>

namespace gw::i18n {

struct LocaleChanged {
    std::string from{};
    std::string to{};
};

struct TableLoaded {
    std::string bcp47{};
    std::uint32_t string_count{0};
    std::uint32_t blob_bytes{0};
    bool          has_bidi_hint{false};
};

struct TableLoadFailed {
    std::string bcp47{};
    std::string path{};
    std::uint8_t error_code{0};
};

} // namespace gw::i18n
