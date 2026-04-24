#pragma once
// engine/render/authoring_lighting.hpp — canonical lighting POD for editor + cook + runtime (plan Track C1).
//
// `gw::editor::render` aliases these types so LightingPanel and future baked
// probes share one layout end-to-end.

#include <array>
#include <cstddef>
#include <cstring>

namespace gw::render::authoring {

struct DynamicLight {
    float color[3]    = {1.f, 1.f, 1.f};
    float intensity   = 1.0f;
    float position[3] = {0.f, 3.f, 0.f};
    float range       = 10.0f;
    bool  enabled     = true;
};

struct LightingParams {
    float ambient_color[3] = {0.56f, 0.65f, 0.90f};
    float ambient_intensity = 0.25f;
    int   active_light      = 0;
    int   light_count       = 2;
    std::array<DynamicLight, 8> lights{};
};

[[nodiscard]] inline std::size_t lighting_blob_byte_size() noexcept {
    return sizeof(LightingParams);
}

/// Trivial blob round-trip for cook ↔ runtime (no compression). Returns bytes written.
inline std::size_t encode_lighting_blob(const LightingParams& p,
                                        void* dst,
                                        std::size_t dst_bytes) noexcept {
    if (!dst || dst_bytes < sizeof(LightingParams)) return 0;
    std::memcpy(dst, &p, sizeof(LightingParams));
    return sizeof(LightingParams);
}

[[nodiscard]] inline bool decode_lighting_blob(LightingParams* out,
                                               const void* src,
                                               std::size_t src_bytes) noexcept {
    if (!out || src_bytes < sizeof(LightingParams) || !src) return false;
    std::memcpy(out, src, sizeof(LightingParams));
    return true;
}

} // namespace gw::render::authoring
