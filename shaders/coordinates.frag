#version 450

// input ========================================
layout(location = 0) in vec4 inColor;

// output =======================================
layout(location = 0) out vec4 outFragColor;

void main(void) {
    outFragColor = inColor;
}