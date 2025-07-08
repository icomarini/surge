#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    uint vertexStageFlag;
    uint fragmentStageFlag;
}
pushConstants;

layout(location = 0) out vec4 fragColor;

void main()
{
    gl_Position = pushConstants.mvp * vec4(inPosition, 1.0);
    fragColor   = inColor;
}