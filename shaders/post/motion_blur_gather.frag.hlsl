// shaders/post/motion_blur_gather.frag.hlsl — ADR-0079 McGuire reconstruction.

Texture2D<float4>  g_color        : register(t0);
Texture2D<float2>  g_velocity     : register(t1);
Texture2D<float2>  g_neighbor_max : register(t2);
SamplerState       g_lin          : register(s0);

cbuffer Params : register(b0) {
    float2 g_texel;
    uint   g_num_taps;
    uint   g_tile_size;
};

float4 main(float2 uv : TEXCOORD0) : SV_TARGET {
    float2 vmax = g_neighbor_max.SampleLevel(g_lin, uv, 0);
    float4 acc  = g_color.SampleLevel(g_lin, uv, 0);
    float  w    = 1.0;
    [loop] for (uint i = 1; i < g_num_taps; ++i) {
        float t = (float(i) + 0.5) / float(g_num_taps);
        float2 s = uv + (t - 0.5) * vmax * g_texel;
        acc += g_color.SampleLevel(g_lin, s, 0);
        w   += 1.0;
    }
    return acc / w;
}
