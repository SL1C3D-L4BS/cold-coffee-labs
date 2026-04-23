// shaders/post/reality_warp.comp.hlsl — Phase 22 W147 (ADR-0108).
// God Mode / Blasphemy State reality warp. Mirror step (Act II) + gravity invert.

cbuffer RealityWarpCB : register(b0)
{
    float mirror_weight;
    float invert_weight;
    float distortion_pulse;
    float _pad0;
    uint  frame_index;
    uint3 _pad1;
};

Texture2D<float4>   src : register(t0);
RWTexture2D<float4> dst : register(u0);
SamplerState        lin : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint2 dims; dst.GetDimensions(dims.x, dims.y);
    if (tid.x >= dims.x || tid.y >= dims.y) return;

    float2 uv = (float2(tid.xy) + 0.5) / float2(dims);

    uv.x = lerp(uv.x, 1.0 - uv.x, saturate(mirror_weight));
    uv.y = lerp(uv.y, 1.0 - uv.y, saturate(invert_weight));

    if (invert_weight > 0.0)
        uv.y = (uv.y - 0.5) * (1.0 + 0.05 * invert_weight) + 0.5;

    if (distortion_pulse > 0.0) {
        float phase = frame_index * 0.05;
        uv.x += sin(phase + uv.y * 6.2831853) * 0.005 * distortion_pulse;
    }

    dst[tid.xy] = src.SampleLevel(lin, uv, 0);
}
