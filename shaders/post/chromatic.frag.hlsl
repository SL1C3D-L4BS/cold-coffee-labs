// shaders/post/chromatic.frag.hlsl — ADR-0079 Wave 17F.

Texture2D<float4> g_color : register(t0);
SamplerState      g_lin   : register(s0);

cbuffer Params : register(b0) { float g_strength; float3 g_pad; };

float4 main(float2 uv : TEXCOORD0) : SV_TARGET {
    float2 dir = uv - 0.5;
    float r = g_color.SampleLevel(g_lin, uv - dir * g_strength, 0).r;
    float g = g_color.SampleLevel(g_lin, uv,                         0).g;
    float b = g_color.SampleLevel(g_lin, uv + dir * g_strength, 0).b;
    return float4(r, g, b, 1);
}
