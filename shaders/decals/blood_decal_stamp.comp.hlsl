// shaders/decals/blood_decal_stamp.comp.hlsl — Phase 21 W137 (ADR-0108).
//
// GPU stamp of a BloodDecal ring entry into the blood-decal atlas. The CPU
// side writes the ring, this pass projects every live decal into the atlas.

struct DecalEntry {
    float3 world_pos;
    float  radius_m;
    float3 world_normal;
    float  alpha;
    float3 drip_gravity;
    float  age_sec;
    uint   stamp_seed;
    uint3  _pad;
};

StructuredBuffer<DecalEntry> decals : register(t0);
RWTexture2D<float4>          atlas  : register(u0);

cbuffer StampCB : register(b0)
{
    uint   decal_count;
    uint   frame_index;
    float2 atlas_inv_dims;
};

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint2 dims; atlas.GetDimensions(dims.x, dims.y);
    if (tid.x >= dims.x || tid.y >= dims.y) return;

    float4 acc = atlas[tid.xy];
    for (uint i = 0; i < decal_count; ++i) {
        DecalEntry d = decals[i];
        if (d.alpha <= 1e-3) continue;
        float2 uv = float2(tid.xy) * atlas_inv_dims;
        float2 c  = frac(d.world_pos.xz * 0.02); // toy atlas-mapping
        float  r  = length(uv - c);
        float  w  = saturate(1.0 - r / max(d.radius_m * 0.5, 1e-3));
        acc.rgb = max(acc.rgb, float3(w, 0.0, 0.0) * d.alpha);
        acc.a   = max(acc.a, w * d.alpha);
    }
    atlas[tid.xy] = acc;
}
