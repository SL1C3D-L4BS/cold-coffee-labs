// Forward+ fragment shader with clustered lighting
// Performs lighting calculations using clustered light assignment

cbuffer CameraConstants : register(b0)
{
    float4x4 View;
    float4x4 Projection;
    float4x4 InvView;
    float4x4 InvProjection;
    float3 CameraPosition;
    float _padding;
};

cbuffer ClusterConstants : register(b1)
{
    uint3 ClusterCount;
    uint LightCount;
    float2 ScreenSize;
    float ZNear;
    float ZFar;
    float FovY;
};

struct Light
{
    float3 Position;
    float3 Color;
    float Intensity;
    float Radius;
    uint Type;              // 0 = point, 1 = spot, 2 = directional
    float3 Direction;
    float InnerCone;
    float OuterCone;
};

struct Material
{
    float3 Albedo;
    float Metallic;
    float Roughness;
    float AO;
};

// Input from vertex shader
struct PSInput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : WORLD_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
};

// Textures
Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D MetallicRoughnessMap : register(t2);
Texture2D AOMap : register(t3);
SamplerState TextureSampler : register(s0);

// Cluster data buffers
StructuredBuffer<uint> LightGrid : register(t4);
StructuredBuffer<uint> LightList : register(t5);
StructuredBuffer<Light> Lights : register(t6);

// Helper functions
float3 GetNormalFromMap(float2 texCoords, float3 worldPos, float3 normal)
{
    float3 tangentNormal = NormalMap.Sample(TextureSampler, texCoords).xyz * 2.0 - 1.0;
    
    float3 Q1 = ddx(worldPos);
    float3 Q2 = ddy(worldPos);
    float2 st1 = ddx(texCoords);
    float2 st2 = ddy(texCoords);
    
    float3 N = normalize(normal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    
    return normalize(mul(tangentNormal, TBN));
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float LinearDepth(float depth)
{
    return (ZNear * ZFar) / (ZFar - depth * (ZFar - ZNear));
}

uint3 GetClusterIndex(float3 viewPos)
{
    // Convert view position to cluster coordinates
    float zSlice = ClusterCount.z * (1.0 - LinearDepth(viewPos.z) / ZFar);
    
    uint z = (uint)clamp(zSlice, 0, ClusterCount.z - 1);
    
    // Convert screen position to cluster coordinates
    float2 screenPos = (viewPos.xy / viewPos.z) * 0.5 + 0.5; // Simplified
    uint x = (uint)clamp(screenPos.x * ClusterCount.x, 0, ClusterCount.x - 1);
    uint y = (uint)clamp(screenPos.y * ClusterCount.y, 0, ClusterCount.y - 1);
    
    return uint3(x, y, z);
}

float3 CalculateLight(Light light, float3 worldPos, float3 N, float3 V, float3 F0, float3 albedo, float metallic, float roughness)
{
    float3 L = normalize(light.Position - worldPos);
    float3 H = normalize(V + L);
    
    float distance = length(light.Position - worldPos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = light.Color * light.Intensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    
    // Add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    return Lo;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // Sample material properties
    float3 albedo = pow(AlbedoMap.Sample(TextureSampler, input.UV).rgb, float3(2.2, 2.2, 2.2));
    float3 normal = GetNormalFromMap(input.UV, input.WorldPos, input.Normal);
    
    float3 metallicRoughness = MetallicRoughnessMap.Sample(TextureSampler, input.UV).rgb;
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;
    float ao = AOMap.Sample(TextureSampler, input.UV).r;
    
    // Calculate view direction
    float3 V = normalize(CameraPosition - input.WorldPos);
    
    // Calculate reflectance at normal incidence
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    
    // Get cluster index for this fragment
    float3 viewPos = mul(View, float4(input.WorldPos, 1.0)).xyz;
    uint3 clusterIndex = GetClusterIndex(viewPos);
    uint flatClusterIndex = clusterIndex.z * ClusterCount.x * ClusterCount.y +
                           clusterIndex.y * ClusterCount.x +
                           clusterIndex.x;
    
    // Get light data for this cluster
    uint lightGridOffset = LightGrid[flatClusterIndex * 2];
    uint lightGridCount = LightGrid[flatClusterIndex * 2 + 1];
    
    // Accumulate lighting
    float3 Lo = float3(0.0, 0.0, 0.0);
    
    for (uint i = 0; i < lightGridCount; ++i)
    {
        uint lightIndex = LightList[lightGridOffset + i];
        Light light = Lights[lightIndex];
        
        if (light.Type == 2) // Directional light
        {
            float3 L = normalize(-light.Direction);
            float3 H = normalize(V + L);
            
            float3 radiance = light.Color * light.Intensity;
            
            // Cook-Torrance BRDF
            float NDF = DistributionGGX(normal, H, roughness);
            float G = GeometrySmith(normal, V, L, roughness);
            float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
            
            float3 kS = F;
            float3 kD = float3(1.0, 1.0, 1.0) - kS;
            kD *= 1.0 - metallic;
            
            float3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
            float3 specular = numerator / denominator;
            
            float NdotL = max(dot(normal, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
        else // Point or spot light
        {
            Lo += CalculateLight(light, input.WorldPos, normal, V, F0, albedo, metallic, roughness);
        }
    }
    
    // Ambient lighting
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;
    
    float3 color = ambient + Lo;
    
    // HDR tonemapping
    color = color / (color + float3(1.0, 1.0, 1.0));
    
    // Gamma correction
    color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));
    
    return float4(color, 1.0);
}
