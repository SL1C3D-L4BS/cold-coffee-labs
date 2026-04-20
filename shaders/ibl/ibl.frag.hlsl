// IBL fragment shader
// Applies image-based lighting using split-sum approximation

struct PS_INPUT {
    float4 position : SV_POSITION;
    float3 world_pos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 view_pos : TEXCOORD3;
    float3 tangent : TEXCOORD4;
    float3 bitangent : TEXCOORD5;
};

// IBL resources
TextureCube<float4> IrradianceMap : register(t0);
TextureCube<float4> PrefilteredMap : register(t1);
Texture2D<float2> BRDFLUT : register(t2);

// Material textures
Texture2D albedo_texture : register(t3);
Texture2D normal_texture : register(t4);
Texture2D metallic_roughness_texture : register(t5);
Texture2D ao_texture : register(t6);

// Samplers
SamplerState texture_sampler : register(s0);
SamplerState cubemap_sampler : register(s1);

// IBL parameters
cbuffer IBLConstants : register(b0) {
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 InverseViewMatrix;
    float3 CameraPosition;
    uint IBL_Enabled;
    float3 LightDirection;
    float EnvironmentIntensity;
    float EnvironmentRotation;
    uint TonemapType;
    float Exposure;
    float Gamma;
    float VignetteStrength;
    float VignettePower;
    uint UseDithering;
    uint2 padding;
}

// Material parameters
cbuffer MaterialConstants : register(b1) {
    float4 AlbedoColor;
    float Metallic;
    float Roughness;
    float AO;
    float Emissive;
    uint UseAlbedoMap;
    uint UseNormalMap;
    uint UseMetallicRoughnessMap;
    uint UseAO;
}

// Utility functions
float3 get_normal_from_map(float2 uv, float3 world_normal, float3 world_pos, float3 tangent, float3 bitangent) {
    float3 tangent_normal = normal_texture.Sample(texture_sampler, uv).xyz * 2.0 - 1.0;
    
    float3x3 TBN = float3x3(normalize(tangent), normalize(bitangent), normalize(world_normal));
    return normalize(mul(tangent_normal, TBN));
}

// Fresnel function (Schlick approximation)
float3 fresnel_schlick(float cos_theta, float3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

// Fresnel function with roughness for IBL
float3 fresnel_schlick_roughness(float cos_theta, float3 f0, float roughness) {
    float3 rough_f = float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness);
    return f0 + (max(rough_f, f0) - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

// Tonemapping functions
float3 aces_tonemap(float3 color) {
    color = color * 0.59719 + float3(0.07600, 0.09256, 0.07465);
    color = color * 0.35458 + float3(0.04323, 0.09571, 0.04444);
    color = color * 0.27477 + float3(0.00266, 0.00266, 0.00266);
    color = max(color, 0.0);
    color = color * (color * 0.15413 + 0.14513) / (color * (color * 0.15413 + 0.51887) + 0.06711);
    return color;
}

float3 reinhard_tonemap(float3 color) {
    return color / (color + 1.0);
}

float3 filmic_tonemap(float3 color) {
    color = max(color - 0.004, 0.0);
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
}

float3 uncharted2_tonemap(float3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    return color;
}

float3 apply_tonemap(float3 color, uint type) {
    switch (type) {
        case 0u:
            return color;
        case 1u:
            return aces_tonemap(color);
        case 2u:
            return reinhard_tonemap(color);
        case 3u:
            return filmic_tonemap(color);
        case 4u:
            return uncharted2_tonemap(color);
        default:
            return color;
    }
}

float3 apply_dithering(float3 color, uint2 pixel_pos) {
    if (!UseDithering) {
        return color;
    }
    
    // Simple 8x8 Bayer matrix dithering
    static const uint bayer8x8[64] = {
        0,  32, 8,  40, 2,  34, 10, 42,
        48, 16, 56, 24, 50, 18, 58, 26,
        12, 44, 4,  36, 14, 46, 6,  38,
        60, 28, 52, 20, 62, 30, 54, 22,
        3,  35, 11, 43, 1,  33, 9,  41,
        51, 19, 59, 27, 49, 17, 57, 25,
        15, 47, 7,  39, 13, 45, 5,  37,
        63, 31, 55, 23, 61, 29, 53, 21
    };
    
    uint index = (pixel_pos.y % 8) * 8 + (pixel_pos.x % 8);
    float dither = float(bayer8x8[index]) / 64.0f;
    
    float a = dither * 0.01;
    return color + float3(a, a, a);
}

float3 apply_vignette(float3 color, float2 uv) {
    if (VignetteStrength <= 0.0) {
        return color;
    }
    
    float2 center = float2(0.5, 0.5);
    float2 dist = uv - center;
    float vignette = 1.0 - length(dist) * VignetteStrength;
    vignette = pow(max(0.0, vignette), VignettePower);
    
    return color * vignette;
}

// Rotate environment map around Y axis
float3 rotate_environment(float3 direction, float rotation) {
    float cos_rot = cos(rotation);
    float sin_rot = sin(rotation);
    
    return float3(
        direction.x * cos_rot - direction.z * sin_rot,
        direction.y,
        direction.x * sin_rot + direction.z * cos_rot
    );
}

// Main fragment shader
float4 main(PS_INPUT input) : SV_Target {
    // Sample material textures (optional maps gated by CPU-side flags)
    float3 albedo = AlbedoColor.rgb;
    if (UseAlbedoMap) {
        albedo = albedo_texture.Sample(texture_sampler, input.uv).rgb;
    }
    
    float3 normal = normalize(input.normal);
    if (UseNormalMap) {
        normal = get_normal_from_map(input.uv, input.normal, input.world_pos, input.tangent, input.bitangent);
    }
    
    float metallic = Metallic;
    float roughness = Roughness;
    if (UseMetallicRoughnessMap) {
        float3 metallic_roughness = metallic_roughness_texture.Sample(texture_sampler, input.uv).rgb;
        metallic = metallic_roughness.b;
        roughness = metallic_roughness.g;
    }
    
    float ao = AO;
    if (UseAO) {
        ao = ao_texture.Sample(texture_sampler, input.uv).r;
    }
    
    // Calculate view vector and reflect vector
    float3 v = normalize(CameraPosition - input.world_pos);
    float3 r = reflect(-v, normal);
    
    // Apply environment rotation
    r = rotate_environment(r, EnvironmentRotation);
    
    // Calculate base reflectivity at normal incidence
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    
    float n_dot_v = max(dot(normal, v), 0.0);
    
    // Sample IBL textures
    float3 irradiance = float3(0.0, 0.0, 0.0);
    float3 prefiltered = float3(0.0, 0.0, 0.0);
    float2 brdf = float2(0.0, 0.0);
    
    if (IBL_Enabled != 0u) {
        // Diffuse IBL
        irradiance = IrradianceMap.Sample(cubemap_sampler, normal).rgb * EnvironmentIntensity;
        
        // Specular IBL
        prefiltered = PrefilteredMap.SampleLevel(cubemap_sampler, r, roughness * 4.0).rgb * EnvironmentIntensity;
        
        // BRDF LUT
        brdf = BRDFLUT.Sample(texture_sampler, float2(n_dot_v, roughness));
    }
    
    // Calculate diffuse IBL contribution
    float3 k_s = fresnel_schlick_roughness(n_dot_v, f0, roughness);
    float3 k_d = (1.0 - k_s) * (1.0 - metallic);
    float3 diffuse_ibl = k_d * irradiance * albedo;
    
    // Calculate specular IBL contribution
    float3 specular_ibl = prefiltered * (k_s * brdf.x + brdf.y);
    
    // Combine IBL contributions
    float3 color = diffuse_ibl + specular_ibl;
    
    // Apply ambient occlusion
    color = lerp(color, color * ao, 0.5);
    
    // Add emissive
    color += albedo * Emissive;
    
    // Apply tonemapping
    color = apply_tonemap(color * Exposure, TonemapType);
    
    // Apply gamma correction
    color = pow(color, 1.0 / Gamma);
    
    // Apply vignette
    color = apply_vignette(color, input.uv);
    
    // Apply dithering
    color = apply_dithering(color, (uint2)input.position.xy);
    
    return float4(color, 1.0);
}
