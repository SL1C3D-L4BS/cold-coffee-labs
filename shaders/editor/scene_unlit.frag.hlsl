// editor/scene_unlit — flat colour; push constants carry rgba.

struct Push {
    float4x4 g_model;
    float4   g_color;
};

[[vk::push_constant]] Push g_pc;

float4 main() : SV_TARGET0 {
    return g_pc.g_color;
}
