// PBR-oriented fragment entry for the reference / tooling path (not the
// production deferred or forward+ cluster). This file intentionally stays
// small: albedo is texture-modulated; normal and metallic-roughness samplers
// are declared for binding parity but are not sampled here yet; lighting is
// a constant ambient plus one hard-coded directional term (no StructuredBuffer
// light loop is executed).

// Material properties
struct Material {
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
};

// Light record shape reserved for a future multi-light pass (buffer unused in main()).
struct Light {
    float3 position;
    float3 color;
    float intensity;
    int type;
};

// Bindings
cbuffer MaterialBuffer : register(b0) {
    Material material;
};
Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicRoughnessTexture : register(t2);
StructuredBuffer<Light> lights : register(t3);
SamplerState textureSampler : register(s0);

struct PSInput {
    float4 position : SV_Position;
    float3 worldPos : WORLD_POS;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target {
    // Sample textures
    float3 albedo = material.albedo * albedoTexture.Sample(textureSampler, input.texCoord).rgb;
    float3 normal = normalize(input.normal);
    
    // Minimal lambert-ish term until the real light loop is wired.
    float3 finalColor = albedo * 0.1; // Ambient
    
    // Simple directional light (temporary)
    float3 lightDir = normalize(float3(-0.5, -1.0, -0.3));
    float NdotL = max(dot(normal, -lightDir), 0.0);
    finalColor += albedo * NdotL * float3(1.0, 0.9, 0.8);
    
    return float4(finalColor, 1.0);
}
