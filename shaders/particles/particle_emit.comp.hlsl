// shaders/particles/particle_emit.comp.hlsl — ADR-0077 Wave 17D.

struct Particle {
    float3 position;
    float  age;
    float3 velocity;
    float  lifetime;
    float2 size;
    float  rotation;
    uint   color;
};

RWStructuredBuffer<Particle>  g_particles   : register(u0);
RWStructuredBuffer<uint>       g_counter     : register(u1);

cbuffer EmitParams : register(b0) {
    uint   g_spawn_count;
    uint   g_budget;
    uint2  g_seed;        // xy = frame + emitter index
    float4 g_position;    // xyz + shape
    float4 g_extent;
};

uint mix32(uint x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }
float unit(uint x) { return float(x) / 4294967295.0; }

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    if (tid.x >= g_spawn_count) return;
    uint slot;
    InterlockedAdd(g_counter[0], 1, slot);
    if (slot >= g_budget) return;

    uint s = g_seed.x ^ (tid.x * 0x9E3779B9u) ^ g_seed.y;
    Particle p;
    float3 r;
    r.x = unit(mix32(s)) * 2 - 1;
    r.y = unit(mix32(s ^ 1)) * 2 - 1;
    r.z = unit(mix32(s ^ 2)) * 2 - 1;
    p.position = g_position.xyz + r * g_extent.xyz;
    p.velocity = float3(0, 0, 0);
    p.size     = float2(1, 1);
    p.rotation = 0;
    p.age      = 0;
    p.lifetime = 1.0 + unit(mix32(s ^ 3));
    p.color    = 0xFFFFFFFFu;
    g_particles[slot] = p;
}
