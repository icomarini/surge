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
    uint vertexStageFlag;
    uint fragmentStageFlag;
};

layout(set = 0, binding = 0) uniform Scene
{
    mat4 projection;
    mat4 view;
};

layout(set = 2, binding = 0) readonly buffer JointMatrices
{
    mat4 jointMatrices[];
};

// output =======================================
layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outViewVec;
layout(location = 4) out vec3 outLightVec;

void main()
{
    // pass on
    outTexCoord = inTexCoord;
    outColor    = vec3(1.0f, 1.0f, 1.0f);

    // skinning
    mat4 skin = inJointWeights.x * jointMatrices[int(inJointIndices.x)] +
                inJointWeights.y * jointMatrices[int(inJointIndices.y)] +
                inJointWeights.z * jointMatrices[int(inJointIndices.z)] +
                inJointWeights.w * jointMatrices[int(inJointIndices.w)];
    gl_Position = vec4(inPosition, 1.0) * skin * model * view * projection;

    // light
    vec4 lightPosition = vec4(5.0f, 5.0f, 5.0f, 1.0f);
    outNormal          = inverse(mat3(skin * model * view)) * inNormal;
    vec4 pos           = vec4(inPosition, 1.0) * view;
    outLightVec        = lightPosition.xyz * mat3(view) - pos.xyz;
    outViewVec         = -pos.xyz;
}