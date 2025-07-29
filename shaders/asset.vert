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
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main()
{
    mat4 skinMat = inJointWeights.x * jointMatrices[int(inJointIndices.x)] +
                   inJointWeights.y * jointMatrices[int(inJointIndices.y)] +
                   inJointWeights.z * jointMatrices[int(inJointIndices.z)] +
                   inJointWeights.w * jointMatrices[int(inJointIndices.w)];

    // gl_Position  = pushConstants.mvp * skinMat * vec4(inPosition, 1.0);
    gl_Position  = vec4(inPosition, 1.0) * skinMat * model * view * projection;
    fragTexCoord = inTexCoord;
    fragColor    = inColor;
    fragNormal   = inNormal;
}