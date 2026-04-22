// shaders/particles/particle_compact.comp.hlsl — ADR-0077 Wave 17D stream-compaction.

struct Particle { float3 position; float age; float3 velocity; float lifetime; float2 size; float rotation; uint color; };

RWStructuredBuffer<Particle>  g_in      : register(u0);
RWStructuredBuffer<Particle>  g_out     : register(u1);
RWStructuredBuffer<uint>       g_in_ctr  : register(u2);
RWStructuredBuffer<uint>       g_out_ctr : register(u3);

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    uint n = g_in_ctr[0];
    if (tid.x >= n) return;
    Particle p = g_in[tid.x];
    if (p.age >= p.lifetime) return;
    uint slot;
    InterlockedAdd(g_out_ctr[0], 1, slot);
    g_out[slot] = p;
}
