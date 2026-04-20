// BRDF Look-Up Table compute shader
// Generates a 2D LUT for the split-sum approximation used in IBL

#define BRDF_LUT_SIZE 512

// Output texture
RWTexture2D<float2> BRDFLUT : register(u0);

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

// Geometry function (Smith)
float geometry_schlick_ggx(float n_dot_v, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;
    
    float nom = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;
    
    return nom / denom;
}

float geometry_smith(float3 n, float3 v, float3 l, float roughness) {
    float n_dot_v = max(dot(n, v), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);
    float ggx2 = geometry_schlick_ggx(n_dot_v, roughness);
    float ggx1 = geometry_schlick_ggx(n_dot_l, roughness);
    
    return ggx1 * ggx2;
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

// Fresnel function (Schlick approximation)
float3 fresnel_schlick(float cos_theta, float3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

[numthreads(16, 16, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    uint x = dispatch_thread_id.x;
    uint y = dispatch_thread_id.y;
    
    if (x >= BRDF_LUT_SIZE || y >= BRDF_LUT_SIZE) {
        return;
    }
    
    // Convert to [0, 1] range
    float u = (float)x / (float)(BRDF_LUT_SIZE - 1);
    float rough_coord = (float)y / (float)(BRDF_LUT_SIZE - 1);
    
    // NdotV is the x coordinate; roughness from y
    float n_dot_v = u;
    float roughness = rough_coord;
    
    // View vector (assuming normal is (0, 0, 1))
    float3 view_dir = float3(sqrt(1.0 - n_dot_v * n_dot_v), 0.0, n_dot_v);
    float3 n = float3(0.0, 0.0, 1.0);
    
    float2 scale_bias = float2(0.0, 0.0);
    
    const uint SAMPLE_COUNT = 1024;
    for (uint i = 0; i < SAMPLE_COUNT; ++i) {
        float2 xi = hammersley(i, SAMPLE_COUNT);
        float3 h = importance_sample_ggx(xi, roughness);
        float3 l = normalize(2.0 * dot(view_dir, h) * h - view_dir);
        
        float n_dot_l = max(dot(n, l), 0.0);
        float n_dot_h = max(dot(n, h), 0.0);
        float v_dot_h = max(dot(view_dir, h), 0.0);
        
        if (n_dot_l > 0.0) {
            // Geometry term
            float g = geometry_smith(n, view_dir, l, roughness);
            
            // Geometry visibility term
            float g_vis = (g * v_dot_h) / (n_dot_h * n_dot_v);
            
            // Fresnel term with F0 = 0.04 (dielectric)
            float f = pow(1.0 - v_dot_h, 5.0);
            float fc = (1.0 - f) * (1.0 - 0.04) + f;
            
            scale_bias.x += g_vis * fc;
            scale_bias.y += g_vis * f;
        }
    }
    
    scale_bias /= float(SAMPLE_COUNT);
    
    // Store the result
    BRDFLUT[uint2(x, y)] = scale_bias;
}
