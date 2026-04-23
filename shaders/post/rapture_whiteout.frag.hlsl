// shaders/post/rapture_whiteout.frag.hlsl — Phase 21 W136.
cbuffer RaptureWhiteoutCB : register(b0) { float intensity; float t_elapsed; float2 _pad; };
Texture2D<float4> src : register(t0);
SamplerState      lin : register(s0);

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float3 c    = src.Sample(lin, uv).rgb;
    float  amt  = saturate(intensity);
    float  t    = max(0.0, t_elapsed);
    if (amt <= 0.0 || t <= 0.0) return float4(c, 1.0);
    float rise = 1.0 - exp(-0.6 * t);
    float w    = amt * rise;
    c = lerp(c, float3(1,1,1), w);
    return float4(c, 1.0);
}
