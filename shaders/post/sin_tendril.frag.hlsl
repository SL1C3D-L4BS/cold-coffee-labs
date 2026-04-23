// shaders/post/sin_tendril.frag.hlsl — Phase 21 W136 (ADR-0108).
//
// Radial tendril-vignette additive override on the horror post stack.
// Drives off `r.sin_tendril` * (SinComponent::current / 100).

cbuffer SinTendrilCB : register(b0)
{
    float intensity;
    float sin_current;
    float2 _pad;
};

Texture2D<float4> src : register(t0);
SamplerState      lin : register(s0);

float3 luma_vec(float3 c) { return dot(c, float3(0.2126, 0.7152, 0.0722)).xxx; }

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    float amt = saturate(intensity) * saturate(sin_current / 100.0);
    float3 c  = src.Sample(lin, uv).rgb;
    if (amt <= 0.0) return float4(c, 1.0);

    float2 d  = uv - 0.5;
    float  r2 = dot(d, d);
    float  tendril = exp(-r2 * 4.0);
    float  vig     = (1.0 - tendril) * amt;
    float3 lum = luma_vec(c);
    c = lerp(c, lum * float3(0.35, 0.20, 0.15), vig);
    return float4(c, 1.0);
}
