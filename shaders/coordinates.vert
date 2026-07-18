#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(set = 0, binding = 0) uniform Scene {
    mat4 projection;
    mat4 view;
    vec4 lightColor;
    vec3 lightPosition;
};

// output =======================================
layout(location = 0) out vec4 outColor;

void main(void) {
    gl_Position = vec4(inPosition, 1.0) * model * view * projection;
    outColor    = inColor;
}