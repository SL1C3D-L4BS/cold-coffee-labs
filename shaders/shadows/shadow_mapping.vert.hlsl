// Shadow mapping vertex shader
// Renders geometry from light's perspective for shadow map generation

struct VS_INPUT {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 tangent : TANGENT;
    uint4 bone_indices : BLENDINDICES;
    float4 bone_weights : BLENDWEIGHT;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float depth : TEXCOORD0;
};

// Per-object matrices
cbuffer ObjectConstants : register(b0) {
    float4x4 WorldMatrix;
    float4x4 WorldInverseTranspose;
};

// Per-frame matrices
cbuffer ViewProjectionConstants : register(b1) {
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 ViewProjectionMatrix;
    float3 CameraPosition;
    float pad0;
    float3 LightDirection;
    float pad1;
    float4 CascadeSplits;
    float4 ShadowParams;
};

// For cascaded shadow mapping, we might have multiple view-projection matrices
cbuffer CascadeConstants : register(b2) {
    float4x4 CascadeViewProjections[4];
    uint CurrentCascade;
    uint CascadeCount;
    uint2 padding;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    
    // Transform position to world space
    float4 world_pos = mul(WorldMatrix, float4(input.position, 1.0));
    
    // For cascaded shadow mapping, use cascade-specific view-projection
    float4x4 view_projection;
    if (CurrentCascade < 4) {
        view_projection = CascadeViewProjections[CurrentCascade];
    } else {
        view_projection = ViewProjectionMatrix;
    }
    
    // Transform to clip space from light's perspective
    output.position = mul(view_projection, world_pos);
    
    // Pass depth for potential use in fragment shader
    output.depth = output.position.z;
    
    return output;
}
