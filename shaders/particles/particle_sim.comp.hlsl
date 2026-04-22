// shaders/particles/particle_sim.comp.hlsl — ADR-0077 Wave 17D.

struct Particle {
    float3 position; float age;
    float3 velocity; float lifetime;
    float2 size; float rotation; uint color;
};

RWStructuredBuffer<Particle> g_particles : register(u0);
RWStructuredBuffer<uint>      g_counter   : register(u1);

cbuffer SimParams : register(b0) {
    float  g_dt;
    float3 g_gravity;
    float  g_drag;
    float  g_curl_amp;
    uint   g_octaves;
    uint   g_seed;
};

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    uint n = g_counter[0];
    if (tid.x >= n) return;
    Particle p = g_particles[tid.x];
    p.age += g_dt;
    p.velocity += g_gravity * g_dt;
    p.velocity *= (1.0 - saturate(g_drag) * g_dt);
    p.position += p.velocity * g_dt;
    g_particles[tid.x] = p;
}
