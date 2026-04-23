// shaders/post/horror_post.comp.hlsl — Phase 21 W135 (ADR-0108).
//
// Single-pass horror post stack: chromatic aberration + film grain +
// screen shake, driven from three CVars pushed via the constant buffer
// below. Mirrors the scalar oracle in engine/render/post/horror_post.cpp
// so tests/render/horror_post_visual_test can golden-hash compare.

cbuffer HorrorPostConstants : register(b0)
{
    float  ca_strength;      // r.horror.chromatic_aberration
    float  grain_intensity;  // r.horror.film_grain
    float  shake_amount;     // r.horror.screen_shake
    float  aspect;
    uint   frame_index;
    uint   seed;
    uint2  _pad;
};

Texture2D<float4>   scene_color : register(t0);
RWTexture2D<float4> out_color   : register(u0);
SamplerState        linear_clamp : register(s0);

static const float kInv0xFFFFFFFF = 1.0 / 4294967295.0;

uint hash3(uint x, uint y, uint z)
{
    uint h = x * 0x9E3779B9u + y * 0x85EBCA6Bu + z * 0xC2B2AE35u;
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h;
}

float hash_signed(uint h) { return (float(h) * kInv0xFFFFFFFF) * 2.0 - 1.0; }

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
    uint2 dims;
    out_color.GetDimensions(dims.x, dims.y);
    if (tid.x >= dims.x || tid.y >= dims.y) return;

    const float2 uv = (float2(tid.xy) + 0.5) / float2(dims);

    // Screen shake — per-frame UV wobble.
    const uint s  = seed ^ frame_index;
    const float2 shake = float2(
        hash_signed(hash3(s, 0x17u, 0x3Cu)),
        hash_signed(hash3(s, 0x29u, 0x91u))
    ) * shake_amount * 0.05;

    const float2 suv = uv + shake;

    // Chromatic aberration — per-channel UV offset radial from centre.
    const float2 d     = (suv - 0.5);
    const float2 uv_r  = suv + d * ca_strength * 0.02;
    const float2 uv_b  = suv - d * ca_strength * 0.02;

    float r = scene_color.SampleLevel(linear_clamp, uv_r, 0).r;
    float g = scene_color.SampleLevel(linear_clamp, suv,  0).g;
    float b = scene_color.SampleLevel(linear_clamp, uv_b, 0).b;

    // Film grain.
    uint gx = uint(suv.x * 4096.0);
    uint gy = uint(suv.y * 4096.0);
    float n = hash_signed(hash3(gx, gy, s)) * grain_intensity * 0.15;
    r += n; g += n; b += n;

    out_color[tid.xy] = float4(r, g, b, 1.0);
}
