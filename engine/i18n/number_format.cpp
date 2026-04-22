// engine/i18n/number_format.cpp — null-ICU shim (ADR-0069 §5).

#include "engine/i18n/number_format.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>

namespace gw::i18n {

namespace {

bool locale_starts_with(std::string_view tag, std::string_view prefix) noexcept {
    if (prefix.size() > tag.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(tag[i]))
            != std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

class NullNumberFormat final : public NumberFormat {
public:
    explicit NullNumberFormat(std::string locale) : tag_(std::move(locale)) {}
    [[nodiscard]] std::string format(std::int64_t v) const override {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
        // Insert thousand separators; `ja` / `zh` also use `,` per CLDR defaults.
        std::string digits(buf);
        const bool negative = !digits.empty() && digits[0] == '-';
        const std::size_t start = negative ? 1 : 0;
        std::string out;
        out.reserve(digits.size() + digits.size() / 3);
        if (negative) out.push_back('-');
        const std::string core = digits.substr(start);
        const int n = static_cast<int>(core.size());
        for (int i = 0; i < n; ++i) {
            if (i != 0 && (n - i) % 3 == 0) out.push_back(grouping_sep_());
            out.push_back(core[static_cast<std::size_t>(i)]);
        }
        return out;
    }
    [[nodiscard]] std::string format(double v, std::int32_t frac_digits) const override {
        if (frac_digits < 0)  frac_digits = 0;
        if (frac_digits > 20) frac_digits = 20;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.*f", frac_digits, v);
        std::string s(buf);
        // Split int + frac, group integral part, swap decimal separator.
        std::size_t dot = s.find('.');
        std::string int_part = (dot == std::string::npos) ? s : s.substr(0, dot);
        std::string frac_part = (dot == std::string::npos) ? std::string{} : s.substr(dot + 1);
        const auto grouped = format(std::atoll(int_part.c_str()));
        std::string out = grouped;
        if (!frac_part.empty()) {
            out.push_back(decimal_sep_());
            out += frac_part;
        }
        return out;
    }
    [[nodiscard]] std::string_view locale() const noexcept override { return tag_; }

private:
    char grouping_sep_() const noexcept {
        if (locale_starts_with(tag_, "fr") || locale_starts_with(tag_, "de")) return '.';
        return ',';
    }
    char decimal_sep_() const noexcept {
        if (locale_starts_with(tag_, "fr") || locale_starts_with(tag_, "de")) return ',';
        return '.';
    }

    std::string tag_;
};

class NullDateTimeFormat final : public DateTimeFormat {
public:
    NullDateTimeFormat(std::string locale, std::string skeleton)
        : tag_(std::move(locale)), skeleton_(std::move(skeleton)) {}

    [[nodiscard]] std::string format(std::int64_t unix_ms) const override {
        std::time_t secs = static_cast<std::time_t>(unix_ms / 1000);
        std::tm tm{};
#ifdef _WIN32
        gmtime_s(&tm, &secs);
#else
        gmtime_r(&secs, &tm);
#endif
        char buf[64];
        if (skeleton_ == "y") {
            std::snprintf(buf, sizeof(buf), "%04d", tm.tm_year + 1900);
        } else if (skeleton_ == "yM") {
            std::snprintf(buf, sizeof(buf), "%04d-%02d", tm.tm_year + 1900, tm.tm_mon + 1);
        } else if (skeleton_ == "yMd") {
            std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        } else if (skeleton_ == "Hms") {
            std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        } else {
            std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                            tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        return buf;
    }
    [[nodiscard]] std::string_view locale() const noexcept override { return tag_; }

private:
    std::string tag_;
    std::string skeleton_;
};

} // namespace

std::unique_ptr<NumberFormat> NumberFormat::make(std::string_view bcp47) {
    return std::make_unique<NullNumberFormat>(std::string{bcp47});
}

std::unique_ptr<DateTimeFormat> DateTimeFormat::make(std::string_view bcp47,
                                                       std::string_view skeleton) {
    return std::make_unique<NullDateTimeFormat>(std::string{bcp47}, std::string{skeleton});
}

PluralCategory plural_category(std::string_view bcp47, double quantity) noexcept {
    // Null shim: CLDR 48 cardinal rules for the five shipped locales.
    const double q = std::abs(quantity);
    if (locale_starts_with(bcp47, "ja") || locale_starts_with(bcp47, "zh")) {
        return PluralCategory::Other; // no plural class distinction
    }
    if (locale_starts_with(bcp47, "fr")) {
        // fr: one for {0, 1}, many for large exponents, else other
        if (q == 0.0 || q == 1.0) return PluralCategory::One;
        return PluralCategory::Other;
    }
    // en, de, and the fall-through default.
    if (q == 1.0) return PluralCategory::One;
    return PluralCategory::Other;
}

std::string to_lower_locale(std::string_view utf8, std::string_view /*bcp47*/) {
    std::string out(utf8);
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string to_upper_locale(std::string_view utf8, std::string_view /*bcp47*/) {
    std::string out(utf8);
    for (auto& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

} // namespace gw::i18n
