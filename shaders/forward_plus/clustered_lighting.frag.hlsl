// Forward+ clustered lighting fragment shader
// Uses light clusters from compute culling pass for efficient lighting

#define MAX_LIGHTS_PER_CLUSTER 64

struct Light {
    float3 position;
    float radius;
    float3 color;
    float intensity;
    float3 direction;
    uint type;        // 0 = point, 1 = spot, 2 = directional
    float inner_cone;
    float outer_cone;
    uint padding;
};

struct ClusterData {
    uint offset;
    uint count;
};

struct Material {
    float3 albedo;
    float metallic;
    float3 normal;
    float roughness;
    float3 emissive;
    float ao;
};

// Input from vertex shader
struct PS_INPUT {
    float4 position : SV_POSITION;
    float3 world_pos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 view_pos : TEXCOORD3;
};

// Output
float4 out_color : SV_Target0;

// Resource bindings
StructuredBuffer<Light> Lights : register(t0);
StructuredBuffer<ClusterData> LightGrid : register(t1);
StructuredBuffer<uint> LightIndexList : register(t2);

// Material textures
Texture2D albedo_texture : register(t3);
Texture2D normal_texture : register(t4);
Texture2D metallic_roughness_texture : register(t5);
Texture2D ao_texture : register(t6);

// Samplers
SamplerState texture_sampler : register(s0);

// Cluster parameters
cbuffer ClusterParams : register(b0) {
    float4x4 ViewProjection;
    float4x4 InverseViewProjection;
    float3 CameraPosition;
    uint ClusterCountX;
    uint ClusterCountY;
    uint ClusterCountZ;
    float3 ClusterScale;
    float3 ClusterBias;
    uint LightCount;
    float2 ScreenDimensions;
    float NearPlane;
    float FarPlane;
};

// PBR helper functions
float3 get_normal_from_map(float2 uv, float3 world_normal, float3 world_pos) {
    float3 tangent_normal = normal_texture.Sample(texture_sampler, uv).xyz * 2.0 - 1.0;
    
    float3 Q1 = ddx(world_pos);
    float3 Q2 = ddy(world_pos);
    float2 st1 = ddx(uv);
    float2 st2 = ddy(uv);
    
    float3 N = normalize(world_normal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    
    return normalize(mul(tangent_normal, TBN));
}

float distribution_ggx(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    
    return num / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float geometry_smith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 fresnel_schlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Light calculation functions
float3 calculate_point_light(Light light, float3 world_pos, float3 N, float3 V, float3 F0, Material material) {
    float3 L = normalize(light.position - world_pos);
    float3 H = normalize(V + L);
    
    float distance = length(light.position - world_pos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = light.color * light.intensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, material.roughness);
    float G = geometry_smith(N, V, L, material.roughness);
    float3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
    
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - material.metallic);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    
    float3 diffuse = kD * material.albedo / 3.14159265;
    
    float NdotL = max(dot(N, L), 0.0);
    return (diffuse + specular) * radiance * NdotL;
}

float3 calculate_spot_light(Light light, float3 world_pos, float3 N, float3 V, float3 F0, Material material) {
    float3 L = normalize(light.position - world_pos);
    float3 H = normalize(V + L);
    
    float distance = length(light.position - world_pos);
    float attenuation = 1.0 / (distance * distance);
    
    // Spot light cone calculation
    float cos_angle = dot(-light.direction, L);
    float spot_intensity = 1.0;
    
    if (cos_angle < light.outer_cone) {
        spot_intensity = 0.0;
    } else if (cos_angle < light.inner_cone) {
        float inner_outer_diff = light.inner_cone - light.outer_cone;
        spot_intensity = (cos_angle - light.outer_cone) / inner_outer_diff;
    }
    
    float3 radiance = light.color * light.intensity * attenuation * spot_intensity;
    
    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, material.roughness);
    float G = geometry_smith(N, V, L, material.roughness);
    float3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
    
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - material.metallic);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    
    float3 diffuse = kD * material.albedo / 3.14159265;
    
    float NdotL = max(dot(N, L), 0.0);
    return (diffuse + specular) * radiance * NdotL;
}

float3 calculate_directional_light(Light light, float3 world_pos, float3 N, float3 V, float3 F0, Material material) {
    float3 L = normalize(-light.direction);
    float3 H = normalize(V + L);
    
    float3 radiance = light.color * light.intensity;
    
    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, material.roughness);
    float G = geometry_smith(N, V, L, material.roughness);
    float3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
    
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - material.metallic);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    
    float3 diffuse = kD * material.albedo / 3.14159265;
    
    float NdotL = max(dot(N, L), 0.0);
    return (diffuse + specular) * radiance * NdotL;
}

// Cluster calculation functions
uint3 get_cluster_coords(float3 view_pos) {
    float3 cluster_coords = view_pos * ClusterScale + ClusterBias;
    return uint3(cluster_coords);
}

uint get_cluster_index(uint3 cluster_coords) {
    return cluster_coords.z * ClusterCountX * ClusterCountY + 
           cluster_coords.y * ClusterCountX + 
           cluster_coords.x;
}

float3 view_space_from_screen(float2 screen_pos, float depth) {
    float4 clip_pos = float4(screen_pos * 2.0 - 1.0, depth, 1.0);
    float4 view_pos = mul(InverseViewProjection, clip_pos);
    return view_pos.xyz / view_pos.w;
}

// Main fragment shader
float4 main(PS_INPUT input) : SV_Target {
    // Sample material textures
    float3 albedo = albedo_texture.Sample(texture_sampler, input.uv).rgb;
    float3 normal = get_normal_from_map(input.uv, input.normal, input.world_pos);
    float3 metallic_roughness = metallic_roughness_texture.Sample(texture_sampler, input.uv).rgb;
    float ao = ao_texture.Sample(texture_sampler, input.uv).r;
    
    Material material;
    material.albedo = albedo;
    material.metallic = metallic_roughness.b;
    material.roughness = metallic_roughness.g;
    material.normal = normal;
    material.emissive = float3(0.0, 0.0, 0.0);
    material.ao = ao;
    
    // Calculate view space position
    float2 screen_uv = input.position.xy / ScreenDimensions;
    float view_depth = input.view_pos.z;
    float3 view_pos = view_space_from_screen(screen_uv, view_depth);
    
    // Get cluster coordinates and index
    uint3 cluster_coords = get_cluster_coords(view_pos);
    cluster_coords = min(cluster_coords, uint3(ClusterCountX - 1, ClusterCountY - 1, ClusterCountZ - 1));
    uint cluster_index = get_cluster_index(cluster_coords);
    
    // Get cluster data
    ClusterData cluster_data = LightGrid[cluster_index];
    uint light_count = min(cluster_data.count, MAX_LIGHTS_PER_CLUSTER);
    uint start_offset = cluster_data.offset;
    
    // Calculate lighting
    float3 N = normalize(material.normal);
    float3 V = normalize(CameraPosition - input.world_pos);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), material.albedo, material.metallic);
    
    float3 Lo = float3(0.0, 0.0, 0.0);
    
    // Iterate through lights in this cluster
    for (uint i = 0; i < light_count; i++) {
        uint light_index = LightIndexList[start_offset + i];
        Light light = Lights[light_index];
        
        switch (light.type) {
            case 0: // Point light
                Lo += calculate_point_light(light, input.world_pos, N, V, F0, material);
                break;
            case 1: // Spot light
                Lo += calculate_spot_light(light, input.world_pos, N, V, F0, material);
                break;
            case 2: // Directional light
                Lo += calculate_directional_light(light, input.world_pos, N, V, F0, material);
                break;
        }
    }
    
    // Ambient lighting
    float3 ambient = float3(0.03, 0.03, 0.03) * material.albedo * material.ao;
    
    float3 color = ambient + Lo + material.emissive;
    
    // HDR tonemapping (simple ACES filmic approximation)
    color = color * (color * 0.15 + 0.05);
    color = color * (color * 0.45 + 0.25);
    color = color * (color * 0.40 + 0.20);
    color = color / (color * 0.20 + 1.0);
    
    // Gamma correction
    color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));
    
    return float4(color, 1.0);
}
