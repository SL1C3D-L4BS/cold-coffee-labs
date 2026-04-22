// shaders/particles/particle_draw.vert.hlsl — ADR-0077 Wave 17D indirect draw.

struct Particle { float3 position; float age; float3 velocity; float lifetime; float2 size; float rotation; uint color; };

StructuredBuffer<Particle> g_particles : register(t0);

cbuffer View : register(b0) { float4x4 g_view_proj; float3 g_cam_right; float _pad0; float3 g_cam_up; float _pad1; };

struct VSOut { float4 svpos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };

static const float2 k_corners[4] = { float2(-0.5,-0.5), float2(0.5,-0.5), float2(-0.5,0.5), float2(0.5,0.5) };
static const float2 k_uvs[4]     = { float2(0,1), float2(1,1), float2(0,0), float2(1,0) };

VSOut main(uint vid : SV_VertexID, uint iid : SV_InstanceID) {
    Particle p = g_particles[iid];
    float2 c = k_corners[vid] * p.size;
    float3 world = p.position + g_cam_right * c.x + g_cam_up * c.y;
    VSOut o;
    o.svpos = mul(g_view_proj, float4(world, 1));
    o.uv    = k_uvs[vid];
    o.color = float4(
        ((p.color >> 24) & 0xFF) / 255.0,
        ((p.color >> 16) & 0xFF) / 255.0,
        ((p.color >>  8) & 0xFF) / 255.0,
        ((p.color      ) & 0xFF) / 255.0);
    return o;
}
