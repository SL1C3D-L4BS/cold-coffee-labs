// shaders/decals/decal_project.frag.hlsl — ADR-0078 Wave 17E deferred decals.
// Wronski-2015 screen-space projection with GL_GREATER depth-read-only edge fix.

Texture2D<float>   g_scene_depth  : register(t0);
Texture2D<float4>  g_decal_albedo[] : register(t1); // bindless
Texture2D<float4>  g_decal_normal[] : register(t33); // bindless (second range)
SamplerState       g_samp_lin     : register(s0);

struct DecalInstance { float4x4 world; uint albedo_idx; uint normal_idx; uint flags; uint _pad; };
StructuredBuffer<DecalInstance> g_decals : register(t65);

cbuffer View : register(b0) {
    float4x4 g_view_proj_inv;
    float4   g_viewport;       // xy = 1/size, zw = cluster tile size
};

struct PSIn {
    float4 svpos     : SV_POSITION;
    float3 local_pos : TEXCOORD0;
    nointerpolation uint idx : TEXCOORD1;
};

float4 main(PSIn p) : SV_TARGET {
    DecalInstance d  = g_decals[p.idx];
    float2 screen_uv = p.svpos.xy * g_viewport.xy;
    float  depth     = g_scene_depth.SampleLevel(g_samp_lin, screen_uv, 0);
    float4 ndc       = float4(screen_uv * 2 - 1, depth, 1);
    ndc.y            = -ndc.y;
    float4 world     = mul(g_view_proj_inv, ndc);
    world.xyz       /= world.w;

    float4x4 inv_world = d.world; // caller supplies inverse in world slot for tight registers
    float3 local     = mul(inv_world, float4(world.xyz, 1)).xyz;

    if (any(abs(local) > 0.5)) discard; // edge fix: clip outside unit cube

    float2 uv = local.xz + 0.5;
    return g_decal_albedo[d.albedo_idx].Sample(g_samp_lin, uv);
}
