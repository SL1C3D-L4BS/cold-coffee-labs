#pragma once
// engine/i18n/number_format.hpp — ADR-0069 §2 (null-ICU shim + future ICU).

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace gw::i18n {

enum class PluralCategory : std::uint8_t {
    Zero = 0,
    One,
    Two,
    Few,
    Many,
    Other,
};

[[nodiscard]] PluralCategory plural_category(std::string_view bcp47, double quantity) noexcept;

class NumberFormat {
public:
    virtual ~NumberFormat() = default;
    [[nodiscard]] virtual std::string format(std::int64_t v) const = 0;
    [[nodiscard]] virtual std::string format(double v, std::int32_t frac_digits) const = 0;
    [[nodiscard]] virtual std::string_view locale() const noexcept = 0;

    [[nodiscard]] static std::unique_ptr<NumberFormat> make(std::string_view bcp47);
};

class DateTimeFormat {
public:
    virtual ~DateTimeFormat() = default;
    // ISO-8601 skeleton-ish patterns: "y", "yM", "yMd", "Hms", "yMdHms".
    [[nodiscard]] virtual std::string format(std::int64_t unix_ms) const = 0;
    [[nodiscard]] virtual std::string_view locale() const noexcept = 0;

    [[nodiscard]] static std::unique_ptr<DateTimeFormat> make(std::string_view bcp47,
                                                                std::string_view skeleton);
};

[[nodiscard]] std::string to_lower_locale(std::string_view utf8, std::string_view bcp47);
[[nodiscard]] std::string to_upper_locale(std::string_view utf8, std::string_view bcp47);

} // namespace gw::i18n
