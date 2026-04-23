// shaders/post/tonemap_pq.hlsl — Part C §21 scaffold (ADR-0112b).
//
// SMPTE ST 2084 (PQ) tonemap for HDR10 output. Encodes linear scene-referred
// luminance to the PQ transfer function so HDR10-capable displays render
// highlights accurately. Sibling of the existing Reinhard/ACES SDR paths.

#ifndef GW_TONEMAP_PQ_HLSL
#define GW_TONEMAP_PQ_HLSL

static const float PQ_m1 = 2610.0 / 16384.0;
static const float PQ_m2 = 2523.0 /    32.0;
static const float PQ_c1 = 3424.0 /  4096.0;
static const float PQ_c2 = 2413.0 /   128.0;
static const float PQ_c3 = 2392.0 /   128.0;

// Scene-referred luminance (cd/m^2) -> PQ-encoded non-linear value [0,1].
float pq_oetf(float luminance_nits) {
    float L = max(luminance_nits, 0.0) / 10000.0;
    float num = PQ_c1 + PQ_c2 * pow(L, PQ_m1);
    float den = 1.0  + PQ_c3 * pow(L, PQ_m1);
    return pow(num / den, PQ_m2);
}

float3 pq_oetf3(float3 linear_nits) {
    return float3(pq_oetf(linear_nits.x),
                  pq_oetf(linear_nits.y),
                  pq_oetf(linear_nits.z));
}

// BT.709 -> BT.2020 primaries (for HDR10).
float3 rec709_to_rec2020(float3 c) {
    const float3x3 M = float3x3(
        0.6274040f, 0.3292820f, 0.0433136f,
        0.0690970f, 0.9195400f, 0.0113612f,
        0.0163916f, 0.0880132f, 0.8955950f);
    return mul(M, c);
}

// Main entry — scene-referred linear (BT.709, cd/m^2) -> PQ-encoded Rec.2020.
float3 tonemap_pq(float3 scene_linear_nits) {
    return pq_oetf3(rec709_to_rec2020(scene_linear_nits));
}

#endif // GW_TONEMAP_PQ_HLSL
