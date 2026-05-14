#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoordinate;
layout(location = 3) in vec4 inTangent;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
    uint isLight;
};

layout(set = 0, binding = 0) uniform Scene {
    mat4 projection;
    mat4 view;
    vec4 lightColor;
    vec3 lightPosition;
};

// output =======================================
layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outTangent;

void main(void) {
    gl_Position = vec4(inPosition.xyz, 1.0);
    outPosition = vec4(inPosition, 1.0) * model * view * projection;
    // outNormal   = mat3(inverse(model * view)) * inNormal;
    // outNormal  = vec3(vec4(inNormal, 1.0) * model);
    outNormal  = inNormal;
    outTangent = inTangent;
}