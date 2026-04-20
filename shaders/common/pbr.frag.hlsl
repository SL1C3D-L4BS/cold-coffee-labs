// PBR fragment shader stub for forward+ rendering
// Week 028: Basic PBR shader for reference renderer

// Material properties
struct Material {
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
};

// Light data (will be filled by forward+ system)
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
    
    // Basic lighting calculation (will be replaced by forward+)
    float3 finalColor = albedo * 0.1; // Ambient
    
    // Simple directional light (temporary)
    float3 lightDir = normalize(float3(-0.5, -1.0, -0.3));
    float NdotL = max(dot(normal, -lightDir), 0.0);
    finalColor += albedo * NdotL * float3(1.0, 0.9, 0.8);
    
    return float4(finalColor, 1.0);
}
