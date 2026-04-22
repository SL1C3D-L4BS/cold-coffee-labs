#pragma once
// engine/scene/seq/gwseq_format.hpp
// Greywater sequencer (.gwseq) wire layout — Phase 18-A.
//
// Uses GLM vector / quaternion types already consumed across engine/ (no new
// third-party dependency — see docs/10_APPENDIX_ADRS_AND_REFERENCES.md).

#include "engine/assets/asset_handle.hpp"
#include "engine/ecs/entity.hpp"

#include <cstdint>
#include <type_traits>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace gw::seq {

inline constexpr std::uint32_t kGwseqMagic   = 0x47575351u;  // 'GWSQ'
inline constexpr std::uint32_t kGwseqVersion = 2u;

/// On-disk header for a .gwseq container (little-endian, naturally aligned).
struct GwseqHeader {
    std::uint32_t magic            = kGwseqMagic;
    std::uint32_t version          = kGwseqVersion;
    std::uint32_t track_count      = 0;
    std::uint32_t frame_rate       = 60;
    std::uint32_t duration_frames  = 0;
};

/// Logical track kinds carried in the file header table.
enum class GwseqTrackType : std::uint32_t {
    Transform = 0,
    Event       = 1,
    Float       = 2,
    Bool        = 3,
    EntityRef   = 4,
};

/// Per-track metadata: `keyframe_offset` is a byte offset from the start of
/// the file into the packed keyframe blob for this track.
struct GwseqTrack {
    std::uint32_t    track_id         = 0;
    GwseqTrackType   track_type       = GwseqTrackType::Transform;
    std::uint64_t    keyframe_offset  = 0;
    std::uint32_t    keyframe_count   = 0;
};

/// Interpolation mode for sampled segments.
enum class GwseqInterpolation : std::uint32_t {
    Step   = 0,
    Linear = 1,
    Bezier = 2,
};

/// World-space translation sample (float64 components).
using Vec3_f64 = glm::dvec3;

/// Rotation sample (double-precision on the wire).
using GwseqQuat = glm::dquat;

/// Stable entity reference on the wire (matches `gw::ecs::Entity` layout).
using EntityHandle = gw::ecs::Entity;

template <typename T>
struct GwseqKeyframe;

template <>
struct GwseqKeyframe<Vec3_f64> {
    std::uint32_t       frame         = 0;
    Vec3_f64            value{};
    GwseqInterpolation  interpolation = GwseqInterpolation::Linear;
    glm::vec2           tangent_in{0.f, 0.f};
    glm::vec2           tangent_out{0.f, 0.f};
};

template <>
struct GwseqKeyframe<GwseqQuat> {
    std::uint32_t       frame         = 0;
    GwseqQuat           value{1.0, 0.0, 0.0, 0.0};
    GwseqInterpolation  interpolation = GwseqInterpolation::Linear;
    glm::vec2           tangent_in{0.f, 0.f};
    glm::vec2           tangent_out{0.f, 0.f};
};

template <>
struct GwseqKeyframe<float> {
    std::uint32_t       frame         = 0;
    float               value         = 0.f;
    GwseqInterpolation  interpolation = GwseqInterpolation::Linear;
    glm::vec2           tangent_in{0.f, 0.f};
    glm::vec2           tangent_out{0.f, 0.f};
};

template <>
struct GwseqKeyframe<bool> {
    std::uint32_t       frame         = 0;
    bool                value         = false;
    std::uint8_t        _pad0[3]{0, 0, 0};
    GwseqInterpolation  interpolation = GwseqInterpolation::Step;
    glm::vec2           tangent_in{0.f, 0.f};
    glm::vec2           tangent_out{0.f, 0.f};
};

template <>
struct GwseqKeyframe<EntityHandle> {
    std::uint32_t       frame         = 0;
    EntityHandle        value         = EntityHandle::null();
    GwseqInterpolation  interpolation = GwseqInterpolation::Step;
    glm::vec2           tangent_in{0.f, 0.f};
    glm::vec2           tangent_out{0.f, 0.f};
};

static_assert(std::is_trivially_copyable_v<GwseqHeader>);
static_assert(std::is_trivially_copyable_v<GwseqTrack>);
static_assert(std::is_trivially_copyable_v<Vec3_f64>);
static_assert(std::is_trivially_copyable_v<GwseqQuat>);
static_assert(std::is_trivially_copyable_v<float>);
static_assert(std::is_trivially_copyable_v<bool>);
static_assert(std::is_trivially_copyable_v<EntityHandle>);
static_assert(std::is_trivially_copyable_v<gw::assets::AssetHandle>);

static_assert(std::is_trivially_copyable_v<GwseqKeyframe<Vec3_f64>>);
static_assert(std::is_trivially_copyable_v<GwseqKeyframe<GwseqQuat>>);
static_assert(std::is_trivially_copyable_v<GwseqKeyframe<float>>);
static_assert(std::is_trivially_copyable_v<GwseqKeyframe<bool>>);
static_assert(std::is_trivially_copyable_v<GwseqKeyframe<EntityHandle>>);

}  // namespace gw::seq
