// shaders/post/grain.frag.hlsl — ADR-0079 Wave 17F.

Texture2D<float4> g_color : register(t0);
SamplerState      g_lin   : register(s0);

cbuffer Params : register(b0) { float g_intensity; float g_size_px; float g_frame_seed; float g_pad; };

uint hash3(uint x, uint y, uint z) {
    uint h = x * 0x9E3779B9u + y * 0x85EBCA6Bu + z * 0xC2B2AE35u;
    h ^= h >> 16; h *= 0x7FEB352Du; h ^= h >> 15;
    h *= 0x846CA68Bu; h ^= h >> 16;
    return h;
}

float4 main(float2 uv : TEXCOORD0) : SV_TARGET {
    float4 c = g_color.SampleLevel(g_lin, uv, 0);
    float inv_sz = 1.0 / max(0.5, g_size_px);
    uint px = uint(uv.x * 4096.0 * inv_sz);
    uint py = uint(uv.y * 4096.0 * inv_sz);
    uint pz = uint(g_frame_seed);
    float n = (float(hash3(px, py, pz)) / 4294967295.0) * 2.0 - 1.0;
    return float4(c.rgb + n * g_intensity, c.a);
}
