// shaders/decals/decal_project.vert.hlsl — ADR-0078 Wave 17E deferred decals.

cbuffer View : register(b0) { float4x4 g_view_proj; };

struct DecalInstance {
    float4x4 world;      // unit-cube → world OBB
    uint     albedo_idx;
    uint     normal_idx;
    uint     flags;
    uint     _pad;
};

StructuredBuffer<DecalInstance> g_decals : register(t0);

struct VSOut {
    float4 svpos        : SV_POSITION;
    float3 local_pos    : TEXCOORD0;
    nointerpolation uint idx : TEXCOORD1;
};

static const float3 k_cube[8] = {
    float3(-0.5,-0.5,-0.5), float3( 0.5,-0.5,-0.5),
    float3(-0.5, 0.5,-0.5), float3( 0.5, 0.5,-0.5),
    float3(-0.5,-0.5, 0.5), float3( 0.5,-0.5, 0.5),
    float3(-0.5, 0.5, 0.5), float3( 0.5, 0.5, 0.5),
};

VSOut main(uint vid : SV_VertexID, uint iid : SV_InstanceID) {
    DecalInstance d = g_decals[iid];
    float3 lp = k_cube[vid & 7];
    float4 wp = mul(d.world, float4(lp, 1));
    VSOut o;
    o.svpos     = mul(g_view_proj, wp);
    o.local_pos = lp;
    o.idx       = iid;
    return o;
}
