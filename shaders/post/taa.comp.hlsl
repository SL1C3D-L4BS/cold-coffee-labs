// shaders/post/taa.comp.hlsl — ADR-0079 Wave 17F.
// Temporal anti-aliasing with k-DOP neighborhood clipping.
// gw_axis: TAA_CLIP_MODE=[kdop,aabb,variance]

Texture2D<float4>  g_curr    : register(t0);
Texture2D<float4>  g_hist    : register(t1);
Texture2D<float2>  g_velocity : register(t2);
SamplerState       g_lin      : register(s0);
RWTexture2D<float4> g_out     : register(u0);

cbuffer Params : register(b0) {
    float2 g_texel;        // 1.0 / dims
    float  g_blend;        // history weight
    float  g_pad;
};

float3 kdop_clip(float3 h, float3 nb[9]) {
    // k-DOP over 7 axes (3 RGB + 4 diagonals); t-min along history-center dir.
    float3 center = nb[4];
    float3 dir    = h - center;
    static const float3 axes[7] = {
        float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1),
        float3( 0.57735,  0.57735,  0.57735),
        float3( 0.57735,  0.57735, -0.57735),
        float3( 0.57735, -0.57735,  0.57735),
        float3(-0.57735,  0.57735,  0.57735),
    };
    float t = 1.0;
    [unroll] for (int i = 0; i < 7; ++i) {
        float dd = dot(dir, axes[i]);
        if (abs(dd) < 1e-6) continue;
        float lo = 1e30, hi = -1e30;
        [unroll] for (int j = 0; j < 9; ++j) {
            float p = dot(nb[j] - center, axes[i]);
            lo = min(lo, p); hi = max(hi, p);
        }
        float t_hi = dd > 0 ? hi / dd : lo / dd;
        t = min(t, max(0.0, t_hi));
    }
    return center + dir * saturate(t);
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    uint2 dims;
    g_out.GetDimensions(dims.x, dims.y);
    if (tid.x >= dims.x || tid.y >= dims.y) return;

    float2 uv = (float2(tid.xy) + 0.5) * g_texel;
    float2 v  = g_velocity.SampleLevel(g_lin, uv, 0);
    float3 h  = g_hist.SampleLevel(g_lin, uv - v, 0).rgb;

    float3 nb[9];
    int idx = 0;
    [unroll] for (int dy = -1; dy <= 1; ++dy)
    [unroll] for (int dx = -1; dx <= 1; ++dx)
        nb[idx++] = g_curr.SampleLevel(g_lin, uv + float2(dx, dy) * g_texel, 0).rgb;

    h = kdop_clip(h, nb);
    float3 c = nb[4];
    g_out[tid.xy] = float4(lerp(c, h, g_blend), 1);
}
