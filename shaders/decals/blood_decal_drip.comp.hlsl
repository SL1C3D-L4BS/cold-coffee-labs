// shaders/decals/blood_decal_drip.comp.hlsl — Phase 21 W137 (ADR-0108).
//
// Drip pass — advects vertical streaks away from the stamp centre under
// the decal's drip_gravity. Runs once per frame after the stamp pass.

RWTexture2D<float4> atlas : register(u0);

cbuffer DripCB : register(b0)
{
    float dt_sec;
    float gravity_scale;
    uint  frame_index;
    uint  _pad;
};

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint2 dims; atlas.GetDimensions(dims.x, dims.y);
    if (tid.x >= dims.x || tid.y >= dims.y) return;

    int2 src = int2(tid.xy) - int2(0, 1);
    if (src.y < 0) return;
    float4 above   = atlas.Load(int3(src, 0));
    float4 current = atlas[tid.xy];
    float  drip    = saturate(dt_sec * gravity_scale);
    atlas[tid.xy]  = lerp(current, max(current, above * 0.85), drip);
}
