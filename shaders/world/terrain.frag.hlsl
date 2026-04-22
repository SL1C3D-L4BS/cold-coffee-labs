// shaders/world/terrain.frag.hlsl — GPTM terrain pixel stage (DXC → SPIR-V).

#include "../../engine/render/shaders/terrain.hlsl"

struct PsIn {
    float4 clip : SV_POSITION;
    float3 world_pos : TEXCOORD0;
    float3 world_nrm : TEXCOORD1;
    float  cam_dist : TEXCOORD2;
    uint   biome_id : TEXCOORD3;
};

float4 main(PsIn pin) : SV_Target0 {
    const uint idx = min(pin.biome_id, 9u);
    float3     albedo = kBiomePalette[idx];

    const float fog_t = saturate((pin.cam_dist - fog_near_m) / max(fog_far_m - fog_near_m, 1e-3));
    albedo = lerp(albedo, fog_color, fog_t);

    const float3 L = normalize(float3(0.35, 1.0, 0.2));
    const float  ndl = saturate(dot(normalize(pin.world_nrm), L));
    const float3 lit = albedo * (0.15 + 0.85 * ndl);
    return float4(lit, 1.0);
}
