// shaders/post/tonemap_aces.frag.hlsl — ADR-0080 (opt-in alt).
// Narkowicz 2015 ACES fit (single-instruction approximation).

Texture2D<float4> g_color : register(t0);
SamplerState      g_lin   : register(s0);

cbuffer Params : register(b0) { float g_exposure; float3 g_pad; };

float3 aces(float3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(float2 uv : TEXCOORD0) : SV_TARGET {
    float3 c = g_color.SampleLevel(g_lin, uv, 0).rgb * g_exposure;
    return float4(aces(c), 1.0);
}
