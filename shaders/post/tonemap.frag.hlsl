// Tonemapping post-processing shader
// Applies tonemapping, gamma correction, and other post-processing effects

struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// Input HDR color buffer
Texture2D<float4> HDRColor : register(t0);

// Optional bloom texture
Texture2D<float4> BloomTexture : register(t1);

// Optional dithering texture
Texture2D<float4> DitherTexture : register(t2);

// Samplers
SamplerState texture_sampler : register(s0);
SamplerState point_sampler : register(s1);

// Tonemapping parameters
cbuffer TonemapConstants : register(b0) {
    uint TonemapType;
    float Exposure;
    float Gamma;
    float BloomStrength;
    float VignetteStrength;
    float VignettePower;
    float Contrast;
    float Saturation;
    uint UseDithering;
    uint UseBloom;
    uint UseVignette;
    uint UseColorGrading;
    float4 ColorGradingLUT; // (offset, scale, 0, 0)
    float2 padding;
}

// Tonemapping functions
float3 aces_tonemap(float3 color) {
    // ACES filmic tonemapper approximation
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
    // Filmic tonemapper (Unreal engine style)
    color = max(color - 0.004, 0.0);
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return color;
}

float3 uncharted2_tonemap(float3 color) {
    // Uncharted 2 tonemapper
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

float3 apply_gamma(float3 color, float gamma) {
    return pow(color, 1.0 / gamma);
}

float3 apply_contrast(float3 color, float contrast) {
    return (color - 0.5) * contrast + 0.5;
}

float3 apply_saturation(float3 color, float saturation) {
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(luminance, color, saturation);
}

float3 apply_vignette(float3 color, float2 uv, float strength, float power) {
    if (strength <= 0.0) {
        return color;
    }
    
    float2 center = float2(0.5, 0.5);
    float2 dist = uv - center;
    float vignette = 1.0 - length(dist) * strength;
    vignette = pow(max(0.0, vignette), power);
    
    return color * vignette;
}

float3 apply_dithering(float3 color, float2 uv) {
    if (UseDithering == 0u) {
        return color;
    }
    
    // Simple 8x8 Bayer matrix dithering using texture coordinates
    float2 dither_uv = uv * 8.0;
    float2 dither_pos = frac(dither_uv);
    
    // Generate dither pattern
    float dither = (dither_pos.x + dither_pos.y) * 0.5;
    
    float a = dither * 0.01;
    return color + float3(a, a, a);
}

float3 apply_color_grading(float3 color) {
    if (UseColorGrading == 0u) {
        return color;
    }
    
    // Simple color grading using LUT parameters
    // In practice, this would sample a 3D LUT texture
    color = color * ColorGradingLUT.y + ColorGradingLUT.x;
    
    return saturate(color);
}

// Main fragment shader
float4 main(PS_INPUT input) : SV_Target {
    // Sample HDR color
    float3 color = HDRColor.Sample(texture_sampler, input.uv).rgb;
    
    // Add bloom if enabled (texture bound when CPU sets UseBloom)
    if (UseBloom != 0u) {
        float3 bloom = BloomTexture.Sample(texture_sampler, input.uv).rgb;
        color += bloom * BloomStrength;
    }
    
    // Apply exposure
    color *= Exposure;
    
    // Apply tonemapping
    color = apply_tonemap(color, TonemapType);
    
    // Apply gamma correction
    color = apply_gamma(color, Gamma);
    
    // Apply contrast
    color = apply_contrast(color, Contrast);
    
    // Apply saturation
    color = apply_saturation(color, Saturation);
    
    // Apply color grading
    color = apply_color_grading(color);
    
    // Apply vignette
    if (UseVignette) {
        color = apply_vignette(color, input.uv, VignetteStrength, VignettePower);
    }
    
    // Apply dithering
    color = apply_dithering(color, input.uv);
    
    return float4(color, 1.0);
}
