// shaders/material/pbr_opaque.vert.hlsl — ADR-0075 Wave 17B.
// gw_axis: PBR_EXT_CLEARCOAT=[0,1]
// gw_axis: PBR_EXT_SPECULAR=[0,1]
// gw_axis: PBR_EXT_SHEEN=[0,1]
// gw_axis: PBR_EXT_TRANSMISSION=[0,1]
// gw_axis: MATERIAL_QUALITY=[low,med,high]

cbuffer View : register(b0) { float4x4 g_view_proj; };
cbuffer Model : register(b1) { float4x4 g_model; };

struct VSIn {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
};

struct VSOut {
    float4 svpos    : SV_POSITION;
    float3 world_n  : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
};

VSOut main(VSIn v) {
    VSOut o;
    float4 wp = mul(g_model, float4(v.position, 1));
    o.svpos   = mul(g_view_proj, wp);
    o.world_n = mul((float3x3)g_model, v.normal);
    o.uv      = v.uv;
    o.tangent = v.tangent;
    return o;
}
