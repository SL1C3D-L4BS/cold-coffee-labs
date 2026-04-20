// Cascaded Shadow Mapping vertex shader
// Renders geometry for multiple cascades in a single pass

struct VS_INPUT {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float3 world_pos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
    uint cascade_index : SV_RenderTargetArrayIndex; // For layered rendering
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

// Cascade matrices
cbuffer CascadeConstants : register(b2) {
    float4x4 CascadeViewProjections[4];
    float4x4 CascadeViewMatrices[4];
    float4x4 CascadeProjectionMatrices[4];
    uint CascadeCount;
    uint3 padding;
};

// Geometry shader would normally handle the cascade selection
// For now, we'll use instancing or multiple draw calls

VS_OUTPUT main(VS_INPUT input, uint instance_id : SV_InstanceID) {
    VS_OUTPUT output;
    
    // Transform position to world space
    float4 world_pos = mul(WorldMatrix, float4(input.position, 1.0));
    output.world_pos = world_pos.xyz;
    
    // Transform normal to world space
    output.normal = mul((float3x3)WorldInverseTranspose, input.normal);
    
    // Pass through texcoord
    output.texcoord = input.texcoord;
    
    // Determine which cascade this instance is for
    // In practice, this would be handled by geometry shader or instancing
    output.cascade_index = instance_id % CascadeCount;
    
    // Use cascade-specific view-projection matrix
    float4x4 cascade_view_projection = CascadeViewProjections[output.cascade_index];
    
    // Transform to clip space
    output.position = mul(cascade_view_projection, world_pos);
    
    return output;
}
