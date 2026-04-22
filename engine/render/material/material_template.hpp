#pragma once
// engine/render/material/material_template.hpp — ADR-0075 Wave 17B.

#include "engine/render/material/material_types.hpp"
#include "engine/render/shader/permutation.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace gw::render::material {

// A MaterialTemplate owns:
//   * a PermutationTable slice (the four KHR axes + quality tier),
//   * a parameter-block layout map (key → byte offset + type),
//   * a default ParameterBlock (reset target for fresh instances).
class MaterialTemplate {
public:
    MaterialTemplate() = default;
    explicit MaterialTemplate(MaterialTemplateDesc desc);

    [[nodiscard]] const MaterialTemplateDesc&         desc() const noexcept { return desc_; }
    [[nodiscard]] const shader::PermutationTable&     permutations() const noexcept { return perms_; }
    [[nodiscard]] std::uint32_t                       permutation_count() const noexcept;
    [[nodiscard]] shader::PermutationKey              select_permutation(PbrExtension mask,
                                                                         MaterialQualityTier q) const;

    struct ParamLayoutEntry {
        ParameterValue::Kind kind{ParameterValue::Kind::F32};
        std::uint16_t        offset{0};
        std::uint16_t        size{0};
    };

    [[nodiscard]] bool register_param(std::string key,
                                       ParameterValue::Kind kind);
    [[nodiscard]] const ParamLayoutEntry* find_param(std::string_view key) const noexcept;
    [[nodiscard]] std::uint16_t           next_offset() const noexcept { return next_offset_; }

    [[nodiscard]] const ParameterBlock& default_block() const noexcept { return default_block_; }
    void                                 set_default_param(std::string_view key, ParameterValue v);

    // Clone this template's default parameter block into `dst` (memcpy).
    void populate_default(ParameterBlock& dst) const noexcept;

    // Set one value inside a destination block by layout lookup.
    [[nodiscard]] bool write_param(ParameterBlock& dst,
                                    std::string_view key,
                                    ParameterValue v) const noexcept;

    // Read one value back.
    [[nodiscard]] bool read_param(const ParameterBlock& src,
                                   std::string_view key,
                                   ParameterValue& out) const noexcept;

    [[nodiscard]] const std::unordered_map<std::string, ParamLayoutEntry>&
        layout() const noexcept { return layout_; }

private:
    MaterialTemplateDesc                                    desc_{};
    shader::PermutationTable                                perms_{};
    std::unordered_map<std::string, ParamLayoutEntry>       layout_{};
    ParameterBlock                                          default_block_{};
    std::uint16_t                                           next_offset_{0};

    static std::uint16_t size_of_(ParameterValue::Kind kind) noexcept;
    void seed_axes_();
};

} // namespace gw::render::material
