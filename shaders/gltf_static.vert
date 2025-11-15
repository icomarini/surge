#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inJointIndices;
layout(location = 5) in vec4 inJointWeights;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    vec4 baseColorFactor;
    uint isLight;
    // uint fragmentStageFlag;
};

layout(set = 0, binding = 0) uniform Scene
{
    mat4 projection;
    mat4 view;
};

// output =======================================
layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;

void main()
{
    gl_Position = vec4(inPosition, 1.0) * model * view * projection;
    outTexCoord = inTexCoord;
    outColor    = baseColorFactor.rgb;
    outNormal   = inNormal;
}