#version 450

// Greywater First Light — fragment stage.
// Interpolates the per-vertex Cold Coffee Labs palette across the triangle.

layout(location = 0) in  vec3 v_color;
layout(location = 0) out vec4 o_color;

void main() {
    o_color = vec4(v_color, 1.0);
}
