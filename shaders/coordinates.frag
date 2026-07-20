#version 450

// input ========================================
layout(location = 0) in vec4 inColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
};

// output =======================================
layout(location = 0) out vec4 outFragColor;

void main(void) {
    outFragColor = inColor;
}