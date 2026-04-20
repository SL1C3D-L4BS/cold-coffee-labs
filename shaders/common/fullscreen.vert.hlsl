// Fullscreen vertex shader for post-processing effects
// Week 028: Basic shader for reference renderer

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput output;
    
    // Output directly to clip space
    output.position = float4(input.position.xy, 0.0, 1.0);
    output.texCoord = input.texCoord;
    
    return output;
}
