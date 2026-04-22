// shaders/post/motion_blur_neighbormax.comp.hlsl — ADR-0079 Wave 17F.

Texture2D<float2>  g_tile_max     : register(t0);
RWTexture2D<float2> g_neighbor_max : register(u0);

cbuffer Params : register(b0) { uint2 g_tile_dims; uint2 g_pad; };

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    if (tid.x >= g_tile_dims.x || tid.y >= g_tile_dims.y) return;
    float2 best = 0;
    float  best_len_sq = 0;
    [unroll] for (int dy = -1; dy <= 1; ++dy) {
    [unroll] for (int dx = -1; dx <= 1; ++dx) {
        int2 p = int2(tid.xy) + int2(dx, dy);
        if (any(p < 0) || any(p >= int2(g_tile_dims))) continue;
        float2 v = g_tile_max[uint2(p)];
        float len_sq = dot(v, v);
        if (len_sq > best_len_sq) { best_len_sq = len_sq; best = v; }
    }}
    g_neighbor_max[tid.xy] = best;
}
