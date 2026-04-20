// Shadow mapping fragment shader
// Outputs depth values for shadow map generation

struct PS_INPUT {
    float4 position : SV_POSITION;
    float depth : TEXCOORD0;
};

// For potential alpha testing (e.g., foliage)
Texture2D albedo_texture : register(t0);
SamplerState texture_sampler : register(s0);

// Material properties
cbuffer MaterialConstants : register(b0) {
    float4 AlbedoColor;
    float Metallic;
    float Roughness;
    float AlphaCutoff;
    uint UseAlphaTest;
    uint2 padding;
};

// Shadow parameters
cbuffer ShadowConstants : register(b1) {
    float4 ShadowParams; // (bias, texel_size, 0, 0)
    float3 LightDirection;
    uint UseSlopeScaleBias;
    float SlopeScaleBias;
    float DepthBias;
    uint2 padding2;
};

// Output depth
float main(PS_INPUT input) : SV_DEPTH {
    // Alpha testing for transparent objects
    if (UseAlphaTest) {
        float2 uv = input.position.xy * ShadowParams.y; // texel_size
        float4 albedo = albedo_texture.Sample(texture_sampler, uv);
        if (albedo.a < AlphaCutoff) {
            discard;
        }
    }
    
    // Apply depth bias to prevent shadow acne
    float depth = input.depth;
    
    if (UseSlopeScaleBias) {
        // Calculate slope scale bias based on depth derivative
        float2 depth_deriv = ddx(depth) + ddy(depth);
        float slope_scale = length(depth_deriv) * SlopeScaleBias;
        depth += slope_scale + DepthBias;
    } else {
        // Simple constant bias
        depth += ShadowParams.x;
    }
    
    return depth;
}
