#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inJointIndices;
layout(location = 5) in vec4 inJointWeights;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    uint vertexStageFlag;
    uint fragmentStageFlag;
}
pushConstants;

layout(std430, set = 1, binding = 0) readonly buffer JointMatrices
{
    mat4 jointMatrices[];
};

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main()
{
    mat4 skinMat = inJointWeights.x * jointMatrices[int(inJointIndices.x)] +
                   inJointWeights.y * jointMatrices[int(inJointIndices.y)] +
                   inJointWeights.z * jointMatrices[int(inJointIndices.z)] +
                   inJointWeights.w * jointMatrices[int(inJointIndices.w)];

    gl_Position  = pushConstants.mvp * skinMat * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragColor    = inColor;
    fragNormal   = inNormal;
}