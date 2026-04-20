#version 450

// Greywater First Light — hardcoded screen-space triangle.
// Phase 1 week 004 *First Light* milestone deliverable.
// Compiled to SPIR-V at build time via glslangValidator.

layout(location = 0) out vec3 v_color;

const vec2 k_positions[3] = vec2[3](
    vec2( 0.0, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

const vec3 k_colors[3] = vec3[3](
    vec3(0.91, 0.87, 0.78),  // Crema
    vec3(0.42, 0.47, 0.50),  // Greywater
    vec3(0.36, 0.25, 0.21)   // Brew
);

void main() {
    gl_Position = vec4(k_positions[gl_VertexIndex], 0.0, 1.0);
    v_color     = k_colors[gl_VertexIndex];
}
