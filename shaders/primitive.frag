#version 450

// input ========================================
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
};

// output =======================================
layout(location = 0) out vec4 outFragColor;

void main(void) {
    outFragColor = baseColor;
}