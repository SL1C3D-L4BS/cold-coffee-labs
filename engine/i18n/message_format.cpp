// engine/i18n/message_format.cpp — null-ICU MessageFormat-2 subset.

#include "engine/i18n/message_format.hpp"

#include "engine/i18n/number_format.hpp"

#include <cstdio>
#include <string>

namespace gw::i18n {

namespace {

const FmtArg* find_arg(std::span<const FmtArg> args, std::string_view key) noexcept {
    for (const auto& a : args) {
        if (a.key == key) return &a;
    }
    return nullptr;
}

std::string arg_as_number(std::string_view bcp47, const FmtArg& a, std::int32_t frac_digits = -1) {
    auto nf = NumberFormat::make(bcp47);
    switch (a.kind) {
        case FmtArg::Kind::I64:
            return frac_digits > 0 ? nf->format(static_cast<double>(a.i), frac_digits)
                                    : nf->format(a.i);
        case FmtArg::Kind::F64:
            return nf->format(a.f, frac_digits < 0 ? 2 : frac_digits);
        case FmtArg::Kind::Str:
        default:
            return std::string{a.s};
    }
}

double arg_quantity(const FmtArg& a) noexcept {
    switch (a.kind) {
        case FmtArg::Kind::I64: return static_cast<double>(a.i);
        case FmtArg::Kind::F64: return a.f;
        case FmtArg::Kind::Str: default: return 0.0;
    }
}

std::string_view plural_keyword(PluralCategory c) noexcept {
    switch (c) {
        case PluralCategory::Zero:  return "zero";
        case PluralCategory::One:   return "one";
        case PluralCategory::Two:   return "two";
        case PluralCategory::Few:   return "few";
        case PluralCategory::Many:  return "many";
        case PluralCategory::Other: default: return "other";
    }
}

// Parse the body of `{name, plural, ...}` — split into {kw, subtemplate}
// pairs, handling nested braces.  Returns the subtemplate matching
// `wanted_kw`, or the `other` arm if not found.
std::string_view find_plural_arm(std::string_view body, std::string_view wanted_kw) {
    std::size_t i = 0;
    std::string_view other_arm{};
    while (i < body.size()) {
        while (i < body.size() && (body[i] == ' ' || body[i] == '\t')) ++i;
        std::size_t kw_start = i;
        while (i < body.size() && body[i] != ' ' && body[i] != '{' && body[i] != '\t') ++i;
        std::string_view kw = body.substr(kw_start, i - kw_start);
        while (i < body.size() && (body[i] == ' ' || body[i] == '\t')) ++i;
        if (i >= body.size() || body[i] != '{') break;
        // find matching close
        std::size_t depth = 1;
        std::size_t arm_start = ++i;
        while (i < body.size() && depth > 0) {
            if (body[i] == '{') ++depth;
            else if (body[i] == '}') {
                --depth;
                if (depth == 0) break;
            }
            ++i;
        }
        std::string_view arm = body.substr(arm_start, i - arm_start);
        if (i < body.size()) ++i;
        if (kw == wanted_kw) return arm;
        if (kw == "other")   other_arm = arm;
    }
    return other_arm;
}

// Forward-declared so arms can recurse.
std::string format_message_internal(std::string_view bcp47,
                                     std::string_view pattern,
                                     std::span<const FmtArg> args);

std::string expand_placeholder(std::string_view bcp47,
                                std::string_view body,
                                std::span<const FmtArg> args) {
    // body ≈ "name" | "name, number" | "name, number, :.2"
    //      | "name, plural, one {...} other {...}"
    std::size_t first_comma = body.find(',');
    std::string_view name = (first_comma == std::string_view::npos) ? body : body.substr(0, first_comma);
    while (!name.empty() && name.back() == ' ') name.remove_suffix(1);

    const FmtArg* a = find_arg(args, name);
    if (!a) return std::string{"{"} + std::string{body} + "}";

    if (first_comma == std::string_view::npos) {
        switch (a->kind) {
            case FmtArg::Kind::Str: return std::string{a->s};
            case FmtArg::Kind::I64: {
                char buf[32]; std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(a->i)); return buf;
            }
            case FmtArg::Kind::F64: {
                char buf[32]; std::snprintf(buf, sizeof(buf), "%g", a->f); return buf;
            }
        }
    }

    std::string_view tail = body.substr(first_comma + 1);
    while (!tail.empty() && tail.front() == ' ') tail.remove_prefix(1);
    std::size_t second_comma = tail.find(',');
    std::string_view kind = (second_comma == std::string_view::npos) ? tail : tail.substr(0, second_comma);
    while (!kind.empty() && kind.back() == ' ') kind.remove_suffix(1);

    if (kind == "number") {
        std::int32_t frac = -1;
        if (second_comma != std::string_view::npos) {
            auto opt = tail.substr(second_comma + 1);
            while (!opt.empty() && opt.front() == ' ') opt.remove_prefix(1);
            // support ":.N"
            if (opt.size() >= 3 && opt[0] == ':' && opt[1] == '.') {
                frac = std::atoi(std::string(opt.substr(2)).c_str());
            }
        }
        return arg_as_number(bcp47, *a, frac);
    }
    if (kind == "plural") {
        const double q  = arg_quantity(*a);
        const auto   pc = plural_category(bcp47, q);
        if (second_comma == std::string_view::npos) return {};
        std::string_view arms = tail.substr(second_comma + 1);
        while (!arms.empty() && arms.front() == ' ') arms.remove_prefix(1);
        auto arm = find_plural_arm(arms, plural_keyword(pc));
        if (arm.empty()) return {};
        // In the arm, `#` is shorthand for the formatted number.
        std::string rendered = format_message_internal(bcp47, arm, args);
        std::string final_out;
        final_out.reserve(rendered.size());
        for (char c : rendered) {
            if (c == '#') {
                auto nf = NumberFormat::make(bcp47);
                if (a->kind == FmtArg::Kind::I64) final_out += nf->format(a->i);
                else                                final_out += nf->format(q, 0);
            } else {
                final_out.push_back(c);
            }
        }
        return final_out;
    }
    // Unknown kind → return raw arg value.
    return arg_as_number(bcp47, *a);
}

std::string format_message_internal(std::string_view bcp47,
                                     std::string_view pattern,
                                     std::span<const FmtArg> args) {
    std::string out;
    out.reserve(pattern.size() + 16);
    for (std::size_t i = 0; i < pattern.size();) {
        const char c = pattern[i];
        if (c == '{' && i + 1 < pattern.size() && pattern[i + 1] == '{') {
            out.push_back('{');
            i += 2;
            continue;
        }
        if (c == '}' && i + 1 < pattern.size() && pattern[i + 1] == '}') {
            out.push_back('}');
            i += 2;
            continue;
        }
        if (c == '{') {
            std::size_t depth = 1;
            std::size_t j = i + 1;
            while (j < pattern.size() && depth > 0) {
                if (pattern[j] == '{') ++depth;
                else if (pattern[j] == '}') {
                    --depth;
                    if (depth == 0) break;
                }
                ++j;
            }
            if (depth == 0) {
                out += expand_placeholder(bcp47, pattern.substr(i + 1, j - i - 1), args);
                i = j + 1;
                continue;
            }
        }
        out.push_back(c);
        ++i;
    }
    return out;
}

} // namespace

std::string format_message(std::string_view bcp47,
                            std::string_view pattern,
                            std::span<const FmtArg> args) {
    return format_message_internal(bcp47, pattern, args);
}

} // namespace gw::i18n
