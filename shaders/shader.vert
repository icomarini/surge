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
layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTextureCoordinate;
layout(location = 3) out vec3 outLightPosition;
layout(location = 4) out vec3 outTangent;
// layout(location = 2) out vec3 outColor;

void main() {
    gl_Position          = vec4(inPosition, 1.0) * model * view * projection;
    outPosition          = vec3(vec4(inPosition, 1.0) * model * view);
    outNormal            = mat3(inverse(model * view)) * inNormal;
    outTextureCoordinate = inTextureCoordinate;
    outLightPosition     = vec3(vec4(lightPosition, 1.0) * view);
    // outTangent           = vec4(mat3(inverse(model * view)) * inTangent.xyz, inTangent.w);
    outTangent = vec3(inverse(model * view) * inTangent);

    // outColor    = baseColor;
}