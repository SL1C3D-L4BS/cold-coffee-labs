// editor/scene_unlit — Wave 1C first-geometry pass (unlit, depth-tested).

struct VSIn {
    float3 pos : POSITION;
};

cbuffer Frame : register(b0) {
    float4x4 g_view;
    float4x4 g_proj;
};

struct Push {
    float4x4 g_model;
    float4   g_color;
};

[[vk::push_constant]] Push g_pc;

float4 main(VSIn v) : SV_POSITION {
    const float4 world = mul(g_pc.g_model, float4(v.pos, 1.0));
    const float4 viewp = mul(g_view, world);
    return mul(g_proj, viewp);
}
