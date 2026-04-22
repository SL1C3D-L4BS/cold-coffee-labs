// shaders/particles/particle_draw.frag.hlsl — ADR-0077 Wave 17D.

Texture2D<float4> g_atlas : register(t0);
SamplerState      g_samp  : register(s0);

struct PSIn { float4 svpos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };

float4 main(PSIn p) : SV_TARGET {
    float4 tex = g_atlas.Sample(g_samp, p.uv);
    return tex * p.color;
}
