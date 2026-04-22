// shaders/post/dof_blur.comp.hlsl — ADR-0079 half-res CoC blur.

Texture2D<float4> g_color : register(t0);
Texture2D<float>  g_coc   : register(t1);
SamplerState      g_lin   : register(s0);
RWTexture2D<float4> g_out : register(u0);

cbuffer Params : register(b0) { float2 g_texel; float g_max_coc_px; float g_pad; };

static const float2 kPoisson[8] = {
    float2( 0.00, 0.00), float2( 0.70, 0.00), float2(-0.70, 0.00),
    float2( 0.00, 0.70), float2( 0.00,-0.70), float2( 0.50, 0.50),
    float2(-0.50, 0.50), float2( 0.50,-0.50)
};

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    uint2 dims;
    g_out.GetDimensions(dims.x, dims.y);
    if (tid.x >= dims.x || tid.y >= dims.y) return;
    float2 uv = (float2(tid.xy) + 0.5) * g_texel;
    float  coc = min(g_coc[tid.xy], g_max_coc_px);
    float4 acc = 0; float w = 0;
    [unroll] for (int i = 0; i < 8; ++i) {
        float2 s  = uv + kPoisson[i] * coc * g_texel;
        acc += g_color.SampleLevel(g_lin, s, 0);
        w   += 1.0;
    }
    g_out[tid.xy] = acc / w;
}
