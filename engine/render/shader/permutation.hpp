#pragma once
// engine/render/shader/permutation.hpp — ADR-0074 Wave 17A.
//
// A "permutation" is one cell in the cartesian product of a template's
// orthogonal feature axes. The engine never compiles on demand; at runtime
// a permutation key is resolved to a pre-cooked SPIR-V blob via the
// permutation cache (permutation_cache.hpp).

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gw::render::shader {

// ---- axis description ------------------------------------------------------

// An axis is either a boolean gate (two variants) or an enumeration.
enum class PermutationAxisKind : std::uint8_t {
    Bool = 0,
    Enum = 1,
};

struct PermutationAxis {
    std::string              name;          // e.g. "GW_HAS_CLEARCOAT"
    PermutationAxisKind      kind{PermutationAxisKind::Bool};
    std::vector<std::string> variants;      // for Bool: {"OFF","ON"}; for Enum: the labels
};

// Variant index within an axis; 0-based.
using VariantIndex = std::uint16_t;

// A permutation key is a tight little-endian sequence of variant indices,
// one per axis, interpreted as a base-N odometer to produce a single
// u32 key.
struct PermutationKey {
    std::uint32_t bits{0};
    constexpr bool operator==(const PermutationKey&) const noexcept = default;
};

// ---- table ----------------------------------------------------------------

// PermutationTable declares the axis layout for one shader/material template.
// The product of |axis.variants| across all axes is the permutation_count.
// Permutation indices outside [0, permutation_count) are rejected.
class PermutationTable {
public:
    PermutationTable() = default;

    // Append an axis. Returns the axis index.
    std::uint16_t add_bool_axis(std::string name);
    std::uint16_t add_enum_axis(std::string name, std::vector<std::string> variants);

    [[nodiscard]] std::size_t axis_count()       const noexcept { return axes_.size(); }
    [[nodiscard]] std::uint32_t permutation_count() const noexcept;
    [[nodiscard]] std::span<const PermutationAxis> axes() const noexcept {
        return {axes_.data(), axes_.size()};
    }

    // Lookups. `indices.size() == axis_count()`; each value must be
    // in-range for its axis.
    [[nodiscard]] PermutationKey encode(std::span<const VariantIndex> indices) const;
    [[nodiscard]] std::vector<VariantIndex> decode(PermutationKey key) const;

    // True iff key is a valid cell index (0 ≤ bits < permutation_count).
    [[nodiscard]] bool valid(PermutationKey key) const noexcept;

    // Produce the HLSL `#define` string for a given permutation. Used
    // when invoking DXC/Slang.
    [[nodiscard]] std::string defines_for(PermutationKey key) const;

    // Canonicalise a free-form HLSL `// gw_axis:` pragma line into an
    // (axis_name, variants) pair. Returns nullopt on parse failure.
    struct ParsedAxis {
        std::string              name;
        std::vector<std::string> variants;
        bool                     is_bool{false};
    };
    [[nodiscard]] static bool parse_pragma(std::string_view line, ParsedAxis& out);

private:
    std::vector<PermutationAxis> axes_{};
};

// Convenience — build a table from a raw HLSL source by scanning
// `// gw_axis: <NAME> = <A>|<B>|...` pragma lines.
[[nodiscard]] PermutationTable extract_axes_from_hlsl(std::string_view hlsl);

// ---- ceiling check (ADR-0074 §2 per-template ≤ 64) ------------------------

struct PermutationBudget {
    std::uint32_t ceiling{64};
    [[nodiscard]] bool within(const PermutationTable&) const noexcept;
};

} // namespace gw::render::shader
