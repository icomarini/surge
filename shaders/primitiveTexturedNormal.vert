#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

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
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outViewVec;
layout(location = 3) out vec3 outLightVec;

void main(void) {
    gl_Position = vec4(inPosition, 1.0) * model * view * projection;
    outNormal   = mat3(transpose(inverse(model))) * inNormal;
    outTexCoord = inTexCoord;
    vec4 pos    = vec4(inPosition, 1.0) * view;
    outLightVec = lightPosition.xyz * mat3(view) - pos.xyz;
    outViewVec  = -pos.xyz;
}