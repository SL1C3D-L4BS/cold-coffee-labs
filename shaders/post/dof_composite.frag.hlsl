// shaders/post/dof_composite.frag.hlsl — ADR-0079 near/far blend.

Texture2D<float4> g_sharp : register(t0);
Texture2D<float4> g_blur  : register(t1);
Texture2D<float>  g_coc   : register(t2);
SamplerState      g_lin   : register(s0);

cbuffer Params : register(b0) { float g_max_coc_px; float3 g_pad; };

float4 main(float2 uv : TEXCOORD0) : SV_TARGET {
    float4 s = g_sharp.SampleLevel(g_lin, uv, 0);
    float4 b = g_blur.SampleLevel(g_lin, uv, 0);
    float  c = g_coc.SampleLevel(g_lin, uv, 0) / max(1.0, g_max_coc_px);
    return lerp(s, b, saturate(c));
}
