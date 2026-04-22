#pragma once
// engine/i18n/i18n_types.hpp — ADR-0069.

#include <cstdint>
#include <string>
#include <string_view>

namespace gw::i18n {

enum class I18nError : std::uint8_t {
    Ok              = 0,
    BadLocale       = 1,
    TableNotLoaded  = 2,
    KeyMissing      = 3,
    IoError         = 4,
    ParseError      = 5,
    HashMismatch    = 6,
    InvalidUtf8     = 7,
    IcuDisabled     = 8,
    HarfBuzzDisabled = 9,
};

[[nodiscard]] constexpr std::string_view to_string(I18nError e) noexcept {
    switch (e) {
        case I18nError::Ok:              return "ok";
        case I18nError::BadLocale:       return "bad_locale";
        case I18nError::TableNotLoaded:  return "table_not_loaded";
        case I18nError::KeyMissing:      return "key_missing";
        case I18nError::IoError:         return "io_error";
        case I18nError::ParseError:      return "parse_error";
        case I18nError::HashMismatch:    return "hash_mismatch";
        case I18nError::InvalidUtf8:     return "invalid_utf8";
        case I18nError::IcuDisabled:     return "icu_disabled";
        case I18nError::HarfBuzzDisabled:return "harfbuzz_disabled";
    }
    return "unknown";
}

struct LocaleStatus {
    std::string bcp47{};
    std::string source_path{};
    std::uint32_t string_count{0};
    std::uint32_t blob_bytes{0};
    bool          loaded{false};
    bool          has_bidi{false};
};

} // namespace gw::i18n
