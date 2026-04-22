// shaders/post/tonemap_pbr_neutral.frag.hlsl — ADR-0080.
// Khronos PBR Neutral tone map (spec v1.0, 13-line closed form).

Texture2D<float4> g_color : register(t0);
SamplerState      g_lin   : register(s0);

cbuffer Params : register(b0) { float g_exposure; float3 g_pad; };

float3 pbr_neutral(float3 color) {
    const float start_compression = 0.8 - 0.04;
    const float desaturation = 0.15;
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    float peak = max(color.r, max(color.g, color.b));
    if (peak < start_compression) return color;
    float d = 1.0 - start_compression;
    float new_peak = 1.0 - (d * d) / (peak + d - start_compression);
    color *= new_peak / peak;
    float g = 1.0 - 1.0 / (desaturation * (peak - new_peak) + 1.0);
    return lerp(color, float3(new_peak, new_peak, new_peak), g);
}

float4 main(float2 uv : TEXCOORD0) : SV_TARGET {
    float3 c = g_color.SampleLevel(g_lin, uv, 0).rgb * g_exposure;
    return float4(pbr_neutral(c), 1.0);
}
