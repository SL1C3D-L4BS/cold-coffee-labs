// editor/mesh_scene_unlit — flat colour; push matches vert for layout.

struct MeshPush {
    float4x4 g_model;
    float4   g_color;
    float3   g_pos_scale;
    uint     g_first_index;
    float3   g_pos_bias;
    uint     g_index_count;
    uint     g_skin_flags;
    uint     g_joint_count;
    uint     g_palette_base;
};

[[vk::push_constant]] MeshPush g_pc;

float4 main() : SV_TARGET0 {
    return g_pc.g_color;
}
