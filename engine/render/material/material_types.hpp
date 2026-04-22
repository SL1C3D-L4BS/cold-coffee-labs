#pragma once
// engine/render/material/material_types.hpp — ADR-0075 Wave 17B.

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gw::render::material {

// ---- opaque resource handles ---------------------------------------------

struct MaterialTemplateId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const MaterialTemplateId&) const noexcept = default;
};

struct MaterialInstanceId {
    std::uint32_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const MaterialInstanceId&) const noexcept = default;
};

struct TextureHandle {
    std::uint64_t value{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0; }
    constexpr bool operator==(const TextureHandle&) const noexcept = default;
};

// ---- PBR extension gates (orthogonal axes, ADR-0075 §2.3) -----------------

enum class PbrExtension : std::uint32_t {
    None          = 0,
    Clearcoat     = 1u << 0,   // KHR_materials_clearcoat
    Specular      = 1u << 1,   // KHR_materials_specular
    Sheen         = 1u << 2,   // KHR_materials_sheen
    Transmission  = 1u << 3,   // KHR_materials_transmission
};
constexpr PbrExtension operator|(PbrExtension a, PbrExtension b) noexcept {
    return static_cast<PbrExtension>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}
constexpr PbrExtension operator&(PbrExtension a, PbrExtension b) noexcept {
    return static_cast<PbrExtension>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}
constexpr bool has(PbrExtension mask, PbrExtension q) noexcept {
    return (static_cast<std::uint32_t>(mask) & static_cast<std::uint32_t>(q)) != 0;
}

// ---- quality tier (ADR-0075 §5 `mat.quality_tier`) ------------------------

enum class MaterialQualityTier : std::uint8_t { Low = 0, Medium = 1, High = 2 };

[[nodiscard]] std::string_view to_string(MaterialQualityTier) noexcept;
[[nodiscard]] MaterialQualityTier parse_quality(std::string_view) noexcept;

// ---- template description -------------------------------------------------

struct MaterialTemplateDesc {
    std::string         name;                                      // "pbr_opaque/metal_rough"
    PbrExtension        extensions{PbrExtension::None};
    MaterialQualityTier quality{MaterialQualityTier::High};
    std::uint32_t       parameter_block_size_bytes{256};           // ≤ 256 (ADR-0075 §2.2)
    std::uint16_t       texture_slot_count{8};                     // ≤ 16
    std::vector<std::string> parameter_keys;                       // typed via ParameterValue
    bool                double_sided{false};
    bool                alpha_blend{false};
};

// ---- parameter + texture table on an instance -----------------------------

struct ParameterValue {
    enum class Kind : std::uint8_t { F32 = 0, Vec4 = 1, I32 = 2 };
    Kind                     kind{Kind::F32};
    float                    f{0.0f};
    std::array<float, 4>     v{0, 0, 0, 0};
    std::int32_t             i{0};
};

// 256-byte parameter block (hard cap). Exposed as a POD so the cache +
// .gwmat codec can memcpy round-trip deterministically.
struct alignas(16) ParameterBlock {
    std::array<std::byte, 256> bytes{};
    [[nodiscard]] constexpr std::size_t size() const noexcept { return bytes.size(); }
};
static_assert(sizeof(ParameterBlock) == 256, "parameter block is 256 B frozen");

struct TextureSlot {
    TextureHandle handle{};
    std::uint32_t fingerprint{0};   // upper half of BLAKE3 id (ADR-0076 §2)
};

struct MaterialInstanceState {
    MaterialTemplateId  parent{};
    ParameterBlock      block{};
    std::array<TextureSlot, 16> textures{};
    std::uint64_t       content_hash{0};
    std::uint32_t       revision{0};   // bumps on any set_param/set_texture
};

// ---- stats ---------------------------------------------------------------

struct MaterialStats {
    std::uint32_t templates{0};
    std::uint32_t instances{0};
    std::uint32_t instance_high_water{0};
    std::uint64_t param_uploads{0};
    std::uint64_t texture_binds{0};
    std::uint64_t reloads{0};
};

} // namespace gw::render::material
