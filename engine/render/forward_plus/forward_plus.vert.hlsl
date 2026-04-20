// Forward+ vertex shader
// Standard vertex shader with position, normal, and UV data

cbuffer CameraConstants : register(b0)
{
    float4x4 View;
    float4x4 Projection;
    float4x4 InvView;
    float4x4 InvProjection;
    float3 CameraPosition;
    float _padding;
};

cbuffer ModelConstants : register(b1)
{
    float4x4 Model;
    float4x4 NormalMatrix;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : WORLD_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    
    // Transform position to world space
    float4 worldPos = mul(Model, float4(input.Position, 1.0));
    output.WorldPos = worldPos.xyz;
    
    // Transform position to clip space
    output.Position = mul(Projection, mul(View, worldPos));
    
    // Transform normal to world space
    output.Normal = mul((float3x3)NormalMatrix, input.Normal);
    output.Tangent = mul((float3x3)NormalMatrix, input.Tangent);
    output.Bitangent = mul((float3x3)NormalMatrix, input.Bitangent);
    
    // Pass through UV
    output.UV = input.UV;
    
    return output;
}
