#version 450

// input ========================================
layout(location = 0) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
};

layout(set = 1, binding = 0) uniform sampler2D texSampler;

// output =======================================
layout(location = 0) out vec4 outFragColor;

void main(void) {
    outFragColor = texture(texSampler, inTexCoord);
}