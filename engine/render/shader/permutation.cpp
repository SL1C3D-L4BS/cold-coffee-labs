// engine/render/shader/permutation.cpp — ADR-0074 Wave 17A.

#include "engine/render/shader/permutation.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace gw::render::shader {

std::uint16_t PermutationTable::add_bool_axis(std::string name) {
    PermutationAxis axis;
    axis.name = std::move(name);
    axis.kind = PermutationAxisKind::Bool;
    axis.variants = {"OFF", "ON"};
    axes_.push_back(std::move(axis));
    return static_cast<std::uint16_t>(axes_.size() - 1);
}

std::uint16_t PermutationTable::add_enum_axis(std::string name,
                                               std::vector<std::string> variants) {
    PermutationAxis axis;
    axis.name = std::move(name);
    axis.kind = PermutationAxisKind::Enum;
    axis.variants = std::move(variants);
    if (axis.variants.empty()) axis.variants.push_back("DEFAULT");
    axes_.push_back(std::move(axis));
    return static_cast<std::uint16_t>(axes_.size() - 1);
}

std::uint32_t PermutationTable::permutation_count() const noexcept {
    if (axes_.empty()) return 1;
    std::uint64_t p = 1;
    for (const auto& a : axes_) {
        p *= a.variants.empty() ? 1u : a.variants.size();
        if (p > 0xFFFF'FFFFull) return 0xFFFF'FFFFu;
    }
    return static_cast<std::uint32_t>(p);
}

PermutationKey PermutationTable::encode(std::span<const VariantIndex> indices) const {
    if (indices.size() != axes_.size()) return PermutationKey{~std::uint32_t{0}};
    std::uint64_t bits = 0;
    std::uint64_t stride = 1;
    for (std::size_t i = 0; i < axes_.size(); ++i) {
        const std::uint64_t radix = axes_[i].variants.size();
        if (radix == 0 || indices[i] >= radix) {
            return PermutationKey{~std::uint32_t{0}};
        }
        bits += static_cast<std::uint64_t>(indices[i]) * stride;
        stride *= radix;
    }
    if (bits > 0xFFFF'FFFFull) return PermutationKey{~std::uint32_t{0}};
    return PermutationKey{static_cast<std::uint32_t>(bits)};
}

std::vector<VariantIndex> PermutationTable::decode(PermutationKey key) const {
    std::vector<VariantIndex> out(axes_.size(), 0);
    std::uint64_t bits = key.bits;
    for (std::size_t i = 0; i < axes_.size(); ++i) {
        const std::uint64_t radix = std::max<std::size_t>(axes_[i].variants.size(), 1);
        out[i] = static_cast<VariantIndex>(bits % radix);
        bits /= radix;
    }
    return out;
}

bool PermutationTable::valid(PermutationKey key) const noexcept {
    return key.bits < permutation_count();
}

std::string PermutationTable::defines_for(PermutationKey key) const {
    const auto indices = decode(key);
    std::string out;
    for (std::size_t i = 0; i < axes_.size(); ++i) {
        const auto& axis = axes_[i];
        if (axis.kind == PermutationAxisKind::Bool) {
            out += "-D";
            out += axis.name;
            out += "=";
            out += (indices[i] == 0) ? "0" : "1";
            out += ' ';
        } else {
            out += "-D";
            out += axis.name;
            out += "_";
            out += axis.variants[indices[i]];
            out += "=1 ";
        }
    }
    if (!out.empty()) out.pop_back();
    return out;
}

bool PermutationTable::parse_pragma(std::string_view line, ParsedAxis& out) {
    // Accepted forms:
    //   // gw_axis: GW_HAS_CLEARCOAT = bool
    //   // gw_axis: QUALITY = low|med|high
    //   /// gw_axis: GW_HAS_SHEEN = bool
    auto trim = [](std::string_view s) {
        while (!s.empty() && (s.front()==' '||s.front()=='\t')) s.remove_prefix(1);
        while (!s.empty() && (s.back() ==' '||s.back() =='\t')) s.remove_suffix(1);
        return s;
    };
    line = trim(line);
    if (line.rfind("//",0)==0) line.remove_prefix(2);
    line = trim(line);
    if (line.rfind("/",0)==0) line.remove_prefix(1);
    line = trim(line);
    constexpr std::string_view kTag = "gw_axis:";
    if (line.substr(0, kTag.size()) != kTag) return false;
    line.remove_prefix(kTag.size());
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) return false;
    out.name = std::string{trim(line.substr(0, eq))};
    auto rhs = trim(line.substr(eq + 1));
    if (out.name.empty() || rhs.empty()) return false;
    out.variants.clear();
    if (rhs == "bool") {
        out.is_bool = true;
        out.variants = {"OFF", "ON"};
        return true;
    }
    out.is_bool = false;
    std::string current;
    for (char c : rhs) {
        if (c == '|') {
            auto v = std::string{trim(current)};
            if (!v.empty()) out.variants.push_back(std::move(v));
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (auto tail = std::string{trim(current)}; !tail.empty()) {
        out.variants.push_back(std::move(tail));
    }
    return !out.variants.empty();
}

PermutationTable extract_axes_from_hlsl(std::string_view hlsl) {
    PermutationTable t;
    std::size_t line_start = 0;
    for (std::size_t i = 0; i <= hlsl.size(); ++i) {
        if (i == hlsl.size() || hlsl[i] == '\n') {
            const auto line = hlsl.substr(line_start, i - line_start);
            PermutationTable::ParsedAxis a;
            if (PermutationTable::parse_pragma(line, a)) {
                if (a.is_bool) t.add_bool_axis(std::move(a.name));
                else           t.add_enum_axis(std::move(a.name), std::move(a.variants));
            }
            line_start = i + 1;
        }
    }
    return t;
}

bool PermutationBudget::within(const PermutationTable& t) const noexcept {
    return t.permutation_count() <= ceiling;
}

} // namespace gw::render::shader
