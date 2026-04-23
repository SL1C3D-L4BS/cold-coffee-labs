// shaders/post/ruin_desat.frag.hlsl — Phase 21 W136.
cbuffer RuinDesatCB : register(b0) { float intensity; float ruin_strength; float2 _pad; };
Texture2D<float4> src : register(t0);
SamplerState      lin : register(s0);

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float3 c   = src.Sample(lin, uv).rgb;
    float  amt = saturate(intensity) * saturate(ruin_strength);
    if (amt <= 0.0) return float4(c, 1.0);
    float  lum = dot(c, float3(0.2126, 0.7152, 0.0722));
    c = lerp(c, float3(lum * 0.95, lum, lum * 1.08), amt);
    return float4(c, 1.0);
}
