// shaders/post/dof_coc.comp.hlsl — ADR-0079 Wave 17F: circle-of-confusion.

Texture2D<float>    g_depth : register(t0);
RWTexture2D<float>  g_coc   : register(u0);

cbuffer Params : register(b0) {
    float g_focal_m;
    float g_aperture_m;
    float g_focus_m;
    float g_px_per_m;
};

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    uint2 dims;
    g_coc.GetDimensions(dims.x, dims.y);
    if (tid.x >= dims.x || tid.y >= dims.y) return;
    float d = g_depth[tid.xy];
    float denom = max(1e-6, d * (g_focus_m - g_focal_m));
    float coc_m = abs((g_focal_m * (g_focus_m - d)) / denom * g_aperture_m);
    g_coc[tid.xy] = coc_m * g_px_per_m;
}
