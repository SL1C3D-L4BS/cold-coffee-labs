#pragma once
// engine/i18n/message_format.hpp — ADR-0070 §2.
//
// Null shim supports:
//   * `{name}`                         — direct substitution.
//   * `{name, number}`                 — locale-aware via NumberFormat.
//   * `{name, number, :.N}`            — fixed-frac.
//   * `{name, plural, one {...} other {...}}`  — plural select.
//   * escape via '{{' and '}}'.

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace gw::i18n {

struct FmtArg {
    enum class Kind : std::uint8_t { Str = 0, I64 = 1, F64 = 2 };
    std::string_view key{};
    Kind             kind{Kind::Str};
    std::string_view s{};
    std::int64_t     i{0};
    double           f{0.0};

    static FmtArg of_str(std::string_view k, std::string_view v) noexcept {
        FmtArg a; a.key = k; a.kind = Kind::Str; a.s = v; return a;
    }
    static FmtArg of_i64(std::string_view k, std::int64_t v) noexcept {
        FmtArg a; a.key = k; a.kind = Kind::I64; a.i = v; return a;
    }
    static FmtArg of_f64(std::string_view k, double v) noexcept {
        FmtArg a; a.key = k; a.kind = Kind::F64; a.f = v; return a;
    }
};

[[nodiscard]] std::string format_message(std::string_view bcp47,
                                           std::string_view pattern,
                                           std::span<const FmtArg> args);

} // namespace gw::i18n
