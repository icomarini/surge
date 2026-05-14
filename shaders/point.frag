#version 450

// input ========================================
layout(location = 0) in vec4 inColor;


// output =======================================
layout(location = 0) out vec4 outColor;


void main() {
    outColor = inColor;
}
