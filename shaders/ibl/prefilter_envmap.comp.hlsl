// Prefilter environment map compute shader
// Generates prefiltered cubemap for specular IBL using importance sampling

#define PREFILTER_SIZE 256
#define MAX_MIP_LEVELS 5

// Input environment cubemap
TextureCube<float4> EnvironmentMap : register(t0);
SamplerState EnvironmentSampler : register(s0);

// Output prefiltered cubemap (different mip levels)
RWTexture2DArray<float4> PrefilteredMap : register(u0);

// Push constants
cbuffer PrefilterConstants : register(b0) {
    uint MipLevel;
    uint Face;
    float Roughness;
    uint SampleCount;
    float2 TextureSize;
    uint2 padding;
}

// Utility functions
float radical_inverse_vdc(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 hammersley(uint i, uint N) {
    return float2(float(i) / float(N), radical_inverse_vdc(i));
}

// Importance sampling for GGX distribution
float3 importance_sample_ggx(float2 xi, float roughness) {
    float a = roughness * roughness;
    
    float phi = 2.0 * 3.14159265 * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    
    // Convert spherical to Cartesian coordinates
    float3 h;
    h.x = sin_theta * cos(phi);
    h.y = sin_theta * sin(phi);
    h.z = cos_theta;
    
    return h;
}

// Normal distribution function (GGX)
float distribution_ggx(float3 n, float3 h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h = max(dot(n, h), 0.0);
    float n_dot_h2 = n_dot_h * n_dot_h;
    
    float num = a2;
    float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    
    return num / denom;
}

// Cube face: +X, -X, +Y, -Y, +Z, -Z — NDC.xy in [-1, 1]
float3 get_normal_for_face(uint face, float2 ndc) {
    switch (face) {
        case 0u:
            return normalize(float3(1.0, -ndc.y, -ndc.x));
        case 1u:
            return normalize(float3(-1.0, -ndc.y, ndc.x));
        case 2u:
            return normalize(float3(ndc.x, 1.0, ndc.y));
        case 3u:
            return normalize(float3(ndc.x, -1.0, -ndc.y));
        case 4u:
            return normalize(float3(ndc.x, -ndc.y, 1.0));
        case 5u:
            return normalize(float3(-ndc.x, -ndc.y, -1.0));
        default:
            return float3(0.0, 0.0, 1.0);
    }
}

[numthreads(16, 16, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    uint x = dispatch_thread_id.x;
    uint y = dispatch_thread_id.y;
    
    // Calculate texture size for this mip level
    uint mip_size = PREFILTER_SIZE >> (uint)MipLevel;
    
    if (x >= mip_size || y >= mip_size) {
        return;
    }
    
    // Convert to [0, 1] range
    float u = (float)x / (float)(mip_size - 1);
    float v = (float)y / (float)(mip_size - 1);
    
    // Convert UV to cubemap direction
    float2 ndc = u * 2.0 - 1.0;
    float2 uv = float2(u, v);
    
    // Calculate normal for this face
    float3 n = get_normal_for_face(Face, ndc);
    
    float3 prefiltered_color = float3(0.0, 0.0, 0.0);
    float total_weight = 0.0;
    
    // Importance sampling
    for (uint i = 0; i < SampleCount; ++i) {
        float2 xi = hammersley(i, SampleCount);
        float3 h = importance_sample_ggx(xi, Roughness);
        
        // Convert half vector to world space
        float3 h_world = normalize(h.z * n + 
                                  h.x * normalize(cross(float3(0.0, 1.0, 0.0), n)) + 
                                  h.y * cross(n, normalize(cross(float3(0.0, 1.0, 0.0), n))));
        
        // Calculate view vector (reflect around normal)
        float3 v = normalize(reflect(float3(0.0, 0.0, 1.0), h_world));
        
        // Calculate light vector (same as view for environment mapping)
        float3 l = v;
        
        float n_dot_l = max(dot(n, l), 0.0);
        
        if (n_dot_l > 0.0) {
            // Calculate PDF
            float n_dot_h = max(dot(n, h_world), 0.0);
            float h_dot_v = max(dot(h_world, v), 0.0);
            float d = distribution_ggx(n, h_world, Roughness);
            float pdf = (d * n_dot_h) / (4.0 * h_dot_v) + 0.0001;
            
            // Sample environment map
            float3 sample_color = EnvironmentMap.SampleLevel(EnvironmentSampler, l, 0.0).rgb;
            
            // Apply importance sampling weight
            float weight = n_dot_l / (pdf * SampleCount);
            
            prefiltered_color += sample_color * weight;
            total_weight += weight;
        }
    }
    
    // Normalize result
    prefiltered_color /= total_weight;
    
    // Store result
    PrefilteredMap[uint3(x, y, Face)] = float4(prefiltered_color, 1.0);
}
