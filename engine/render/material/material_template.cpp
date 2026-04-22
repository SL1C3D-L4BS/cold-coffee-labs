// engine/render/material/material_template.cpp — ADR-0075 Wave 17B.

#include "engine/render/material/material_template.hpp"

#include <cstring>
#include <utility>

namespace gw::render::material {

MaterialTemplate::MaterialTemplate(MaterialTemplateDesc d)
    : desc_(std::move(d)) {
    seed_axes_();
    for (const auto& key : desc_.parameter_keys) {
        (void)register_param(key, ParameterValue::Kind::Vec4);
    }
}

void MaterialTemplate::seed_axes_() {
    // Four KHR gates + one enum quality tier.
    // Axis order is *frozen* (ADR-0075 §2.3) so permutation keys are
    // stable across sessions: clearcoat, specular, sheen, transmission,
    // quality.
    (void)perms_.add_bool_axis("GW_HAS_CLEARCOAT");
    (void)perms_.add_bool_axis("GW_HAS_SPECULAR");
    (void)perms_.add_bool_axis("GW_HAS_SHEEN");
    (void)perms_.add_bool_axis("GW_HAS_TRANSMISSION");
    (void)perms_.add_enum_axis("GW_QUALITY", {"LOW", "MEDIUM", "HIGH"});
}

std::uint32_t MaterialTemplate::permutation_count() const noexcept {
    return perms_.permutation_count();
}

shader::PermutationKey MaterialTemplate::select_permutation(PbrExtension mask,
                                                              MaterialQualityTier q) const {
    std::vector<shader::VariantIndex> idx;
    idx.reserve(perms_.axis_count());
    idx.push_back(has(mask, PbrExtension::Clearcoat)    ? 1 : 0);
    idx.push_back(has(mask, PbrExtension::Specular)     ? 1 : 0);
    idx.push_back(has(mask, PbrExtension::Sheen)        ? 1 : 0);
    idx.push_back(has(mask, PbrExtension::Transmission) ? 1 : 0);
    idx.push_back(static_cast<shader::VariantIndex>(q));
    return perms_.encode(idx);
}

std::uint16_t MaterialTemplate::size_of_(ParameterValue::Kind kind) noexcept {
    switch (kind) {
        case ParameterValue::Kind::F32:  return 4;
        case ParameterValue::Kind::Vec4: return 16;
        case ParameterValue::Kind::I32:  return 4;
    }
    return 0;
}

bool MaterialTemplate::register_param(std::string key, ParameterValue::Kind kind) {
    const auto sz = size_of_(kind);
    if (sz == 0) return false;
    if (static_cast<std::uint32_t>(next_offset_) + sz > desc_.parameter_block_size_bytes) return false;
    auto [it, inserted] = layout_.try_emplace(std::move(key), ParamLayoutEntry{});
    if (!inserted) return false;
    it->second.kind   = kind;
    it->second.offset = next_offset_;
    it->second.size   = sz;
    next_offset_      = static_cast<std::uint16_t>(next_offset_ + sz);
    return true;
}

const MaterialTemplate::ParamLayoutEntry*
MaterialTemplate::find_param(std::string_view key) const noexcept {
    auto it = layout_.find(std::string{key});
    if (it == layout_.end()) return nullptr;
    return &it->second;
}

void MaterialTemplate::set_default_param(std::string_view key, ParameterValue v) {
    (void)write_param(default_block_, key, v);
}

void MaterialTemplate::populate_default(ParameterBlock& dst) const noexcept {
    std::memcpy(dst.bytes.data(), default_block_.bytes.data(), dst.bytes.size());
}

bool MaterialTemplate::write_param(ParameterBlock& dst,
                                    std::string_view key,
                                    ParameterValue v) const noexcept {
    const auto* e = find_param(key);
    if (!e) return false;
    if (e->kind != v.kind) return false;
    if (static_cast<std::size_t>(e->offset) + e->size > dst.bytes.size()) return false;
    switch (v.kind) {
        case ParameterValue::Kind::F32:
            std::memcpy(dst.bytes.data() + e->offset, &v.f, sizeof(float));
            break;
        case ParameterValue::Kind::I32:
            std::memcpy(dst.bytes.data() + e->offset, &v.i, sizeof(std::int32_t));
            break;
        case ParameterValue::Kind::Vec4:
            std::memcpy(dst.bytes.data() + e->offset, v.v.data(), 4 * sizeof(float));
            break;
    }
    return true;
}

bool MaterialTemplate::read_param(const ParameterBlock& src,
                                   std::string_view key,
                                   ParameterValue& out) const noexcept {
    const auto* e = find_param(key);
    if (!e) return false;
    if (static_cast<std::size_t>(e->offset) + e->size > src.bytes.size()) return false;
    out.kind = e->kind;
    switch (e->kind) {
        case ParameterValue::Kind::F32:
            std::memcpy(&out.f, src.bytes.data() + e->offset, sizeof(float));
            break;
        case ParameterValue::Kind::I32:
            std::memcpy(&out.i, src.bytes.data() + e->offset, sizeof(std::int32_t));
            break;
        case ParameterValue::Kind::Vec4:
            std::memcpy(out.v.data(), src.bytes.data() + e->offset, 4 * sizeof(float));
            break;
    }
    return true;
}

// ---- free-function helpers (kept out of the type header) -----------------

std::string_view to_string(MaterialQualityTier q) noexcept {
    switch (q) {
        case MaterialQualityTier::Low:    return "low";
        case MaterialQualityTier::Medium: return "med";
        case MaterialQualityTier::High:   return "high";
    }
    return "high";
}

MaterialQualityTier parse_quality(std::string_view s) noexcept {
    if (s == "low") return MaterialQualityTier::Low;
    if (s == "med" || s == "medium") return MaterialQualityTier::Medium;
    return MaterialQualityTier::High;
}

} // namespace gw::render::material
