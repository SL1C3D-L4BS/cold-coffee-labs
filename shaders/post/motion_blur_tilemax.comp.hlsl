// shaders/post/motion_blur_tilemax.comp.hlsl — ADR-0079 Wave 17F (McGuire).

Texture2D<float2>  g_velocity : register(t0);
RWTexture2D<float2> g_tile_max : register(u0);

cbuffer Params : register(b0) {
    uint2 g_dims;
    uint  g_tile_size;
    uint  g_pad;
};

[numthreads(16, 16, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    uint2 tile_dims = (g_dims + g_tile_size - 1) / g_tile_size;
    if (tid.x >= tile_dims.x || tid.y >= tile_dims.y) return;

    float2 best = 0;
    float  best_len_sq = 0;
    uint2 px0 = tid.xy * g_tile_size;
    uint2 px1 = min(px0 + g_tile_size, g_dims);
    [loop] for (uint y = px0.y; y < px1.y; ++y) {
    [loop] for (uint x = px0.x; x < px1.x; ++x) {
        float2 v = g_velocity[uint2(x, y)];
        float len_sq = dot(v, v);
        if (len_sq > best_len_sq) { best_len_sq = len_sq; best = v; }
    }}
    g_tile_max[tid.xy] = best;
}
