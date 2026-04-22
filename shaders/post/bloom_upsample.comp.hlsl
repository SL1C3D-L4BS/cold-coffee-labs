// shaders/post/bloom_upsample.comp.hlsl — ADR-0079 Wave 17F.
// Dual-Kawase up pass: additive 4-tap blur into a 2x-resolution target.

Texture2D<float4>  g_src : register(t0);
SamplerState       g_lin : register(s0);
RWTexture2D<float4> g_dst : register(u0);

cbuffer Params : register(b0) {
    float2 g_dst_texel;
    float2 g_src_texel;
    float  g_offset;   // default 0.5 for upsample
    float  g_intensity; // composite weight
};

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    uint2 dst_dims;
    g_dst.GetDimensions(dst_dims.x, dst_dims.y);
    if (tid.x >= dst_dims.x || tid.y >= dst_dims.y) return;

    float2 uv = (float2(tid.xy) + 0.5) * g_dst_texel;
    float2 o  = g_offset * g_src_texel;

    float4 s = 0;
    s += g_src.SampleLevel(g_lin, uv + float2(-o.x, -o.y), 0);
    s += g_src.SampleLevel(g_lin, uv + float2(+o.x, -o.y), 0);
    s += g_src.SampleLevel(g_lin, uv + float2(-o.x, +o.y), 0);
    s += g_src.SampleLevel(g_lin, uv + float2(+o.x, +o.y), 0);
    g_dst[tid.xy] += s * 0.25 * g_intensity;
}
