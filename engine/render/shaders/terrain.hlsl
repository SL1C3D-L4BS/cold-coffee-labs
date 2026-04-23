// engine/render/shaders/terrain.hlsl — GPTM terrain shared declarations (included by shaders/world/terrain.*.hlsl).
//
// Vertex layout must match `gw::world::gptm::GptmVertex` (32-byte interleaved). Until the Vulkan
// vertex input state is generated from reflection, the entry-point shaders use a layout-equivalent
// field pack: float3 + uint2 oct + float2 uv + uint biome + uint pad.

#ifndef GW_TERRAIN_HLSL
#define GW_TERRAIN_HLSL

cbuffer SceneConstants : register(b0) {
    row_major float4x4 view_proj;
    float3             fog_color;
    float              fog_near_m; // mirrors `world.gptm.scene_fog_near` when plumbed from CVars
    float              fog_far_m;
    float3             camera_world_lo;
    float              _cb_pad0;
    float3             camera_world_hi;
    float              _cb_pad1;
};

struct GptmTileInstanceHlsl {
    float chunk_origin_x_lo;
    float chunk_origin_y_lo;
    float chunk_origin_z_lo;
    float chunk_origin_x_hi;
    float chunk_origin_y_hi;
    float chunk_origin_z_hi;
    uint  biome;
    uint  padding;
};

StructuredBuffer<GptmTileInstanceHlsl> g_tile_instances : register(t0);

static const float3 kBiomePalette[10] = {
    float3(0.02f, 0.02f, 0.05f),  // Obsidian
    float3(0.55f, 0.02f, 0.02f),  // Blood Red
    float3(0.92f, 0.95f, 1.00f),  // Ice White
    float3(0.95f, 0.65f, 0.72f),  // Flesh Pink
    float3(0.95f, 0.45f, 0.08f),  // Forge Orange
    float3(0.45f, 0.45f, 0.48f),  // Ash Grey
    float3(0.45f, 0.12f, 0.65f),  // Heretic Purple
    float3(0.82f, 0.74f, 0.52f),  // Sand
    float3(0.78f, 0.82f, 0.86f),  // Mirror Silver
    float3(0.00f, 0.00f, 0.00f),  // Void Black
};

float3 gptm_decode_oct_normal(uint ox, uint oy) {
    const float x = (ox / 65535.0) * 2.0 - 1.0;
    const float y = (oy / 65535.0) * 2.0 - 1.0;
    float       px  = x;
    float       py  = y;
    float       z   = 1.0 - abs(x) - abs(y);
    if (z < 0.0) {
        px = (x >= 0.0 ? 1.0 : -1.0) - x;
        py = (y >= 0.0 ? 1.0 : -1.0) - y;
    }
    const float3 n = float3(px, py, z);
    const float  l = length(n);
    if (l < 1e-5)
        return float3(0.0, 1.0, 0.0);
    return n / l;
}

float3 reconstruct_camera_world() {
    return camera_world_lo + camera_world_hi;
}

#endif // GW_TERRAIN_HLSL
