// shaders/world/terrain.vert.hlsl — GPTM terrain vertex stage (DXC → SPIR-V).

#include "../../engine/render/shaders/terrain.hlsl"

struct VsIn {
    float3 local_pos : POSITION;
    uint2  normal_oct : TEXCOORD0;
    float2 uv : TEXCOORD1;
    uint biome_id : TEXCOORD2;
    uint3 _pad : TEXCOORD3;
    uint _struct_tail_pad : TEXCOORD4;
};

struct VsOut {
    float4 clip : SV_POSITION;
    float3 world_pos : TEXCOORD0;
    float3 world_nrm : TEXCOORD1;
    float  cam_dist : TEXCOORD2;
    uint   biome_id : TEXCOORD3;
};

VsOut main(VsIn vin, uint inst_id : SV_InstanceID) {
    VsOut o;

    const GptmTileInstanceHlsl inst = g_tile_instances[inst_id];

    const float3 chunk_lo = float3(inst.chunk_origin_x_lo, inst.chunk_origin_y_lo, inst.chunk_origin_z_lo);
    const float3 chunk_hi = float3(inst.chunk_origin_x_hi, inst.chunk_origin_y_hi, inst.chunk_origin_z_hi);
    const float3 world    = vin.local_pos + chunk_lo + chunk_hi;

    const float4 clip_pos = mul(float4(world, 1.0), view_proj);
    o.clip      = clip_pos;
    o.world_pos = world;
    o.world_nrm = gptm_decode_oct_normal(vin.normal_oct.x, vin.normal_oct.y);
    const float3 cam = reconstruct_camera_world();
    o.cam_dist  = length(world - cam);
    o.biome_id  = vin.biome_id;
    return o;
}
