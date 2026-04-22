// shaders/material/pbr_opaque.frag.hlsl — ADR-0075 Wave 17B.
// gw_axis: PBR_EXT_CLEARCOAT=[0,1]
// gw_axis: PBR_EXT_SPECULAR=[0,1]
// gw_axis: PBR_EXT_SHEEN=[0,1]
// gw_axis: PBR_EXT_TRANSMISSION=[0,1]
// gw_axis: MATERIAL_QUALITY=[low,med,high]

Texture2D<float4> g_albedo : register(t0);
Texture2D<float2> g_mr     : register(t1);
Texture2D<float3> g_normal : register(t2);
SamplerState      g_samp   : register(s0);

cbuffer Params : register(b0) {
    float4 g_base_color;
    float4 g_metallic_roughness; // r = metallic, g = roughness
    float4 g_emissive;
#if PBR_EXT_CLEARCOAT
    float4 g_clearcoat;          // r = strength, g = roughness
#endif
#if PBR_EXT_SPECULAR
    float4 g_specular;           // rgb = tint, a = factor
#endif
#if PBR_EXT_SHEEN
    float4 g_sheen_color;        // rgb = color, a = roughness
#endif
#if PBR_EXT_TRANSMISSION
    float4 g_transmission;       // r = factor, g = thickness
#endif
};

struct PSIn {
    float4 svpos   : SV_POSITION;
    float3 world_n : NORMAL;
    float2 uv      : TEXCOORD0;
    float4 tangent : TANGENT;
};

float4 main(PSIn p) : SV_TARGET {
    float4 base = g_albedo.Sample(g_samp, p.uv) * g_base_color;
    float2 mr   = g_mr.Sample(g_samp, p.uv) * g_metallic_roughness.rg;
    float3 n    = normalize(p.world_n);
    float3 l    = normalize(float3(0.3, 0.8, 0.5));
    float  ndl  = saturate(dot(n, l));
    float3 color = base.rgb * ndl * (1.0 - mr.x) + g_emissive.rgb;
#if PBR_EXT_CLEARCOAT
    color += float3(g_clearcoat.r * 0.05, g_clearcoat.r * 0.05, g_clearcoat.r * 0.05);
#endif
#if PBR_EXT_SHEEN
    color += g_sheen_color.rgb * 0.02;
#endif
    return float4(color, base.a);
}
