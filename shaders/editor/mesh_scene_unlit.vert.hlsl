// editor/mesh_scene_unlit — indexed .gwmesh stream0 (int16 xyz + pad) dequant +
// optional stream2 skin (joint indices/weights) + joint matrix palette SSBO.

struct VSIn {
    int4 posq : POSITION;
    uint2 joints01 : TEXCOORD0;
    uint weights255 : TEXCOORD1;
};

[[vk::binding(0, 0)]]
cbuffer Frame : register(b0) {
    float4x4 g_view;
    float4x4 g_proj;
};

[[vk::binding(1, 0)]]
StructuredBuffer<float4x4> g_joint_palette : register(t0);

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

float4 main(VSIn v) : SV_POSITION {
    const float3 p =
        float3(v.posq.xyz) * (1.0 / 32767.0) * g_pc.g_pos_scale + g_pc.g_pos_bias;
    const float4 p_os = float4(p, 1.0);

    float4 deformed = p_os;
    if ((g_pc.g_skin_flags & 1u) != 0u && g_pc.g_joint_count > 0u) {
        const uint j0 = v.joints01.x & 0xFFFFu;
        const uint j1 = v.joints01.x >> 16u;
        const uint j2 = v.joints01.y & 0xFFFFu;
        const uint j3 = v.joints01.y >> 16u;
        float w0 = float(v.weights255 & 255u);
        float w1 = float((v.weights255 >> 8u) & 255u);
        float w2 = float((v.weights255 >> 16u) & 255u);
        float w3 = float((v.weights255 >> 24u) & 255u);
        const float denom = max(w0 + w1 + w2 + w3, 1.0);
        w0 /= denom;
        w1 /= denom;
        w2 /= denom;
        w3 /= denom;
        const uint base = g_pc.g_palette_base;
        const float4x4 m0 = g_joint_palette[base + j0];
        const float4x4 m1 = g_joint_palette[base + j1];
        const float4x4 m2 = g_joint_palette[base + j2];
        const float4x4 m3 = g_joint_palette[base + j3];
        deformed = w0 * mul(m0, p_os) + w1 * mul(m1, p_os) + w2 * mul(m2, p_os) + w3 * mul(m3, p_os);
    }

    const float4 world = mul(g_pc.g_model, deformed);
    const float4 viewp = mul(g_view, world);
    return mul(g_proj, viewp);
}
