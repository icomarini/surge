#version 450


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    uint vertexStageFlag;
    uint fragmentStageFlag;
}
pushConstants;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;

void main()
{
    gl_Position  = pushConstants.mvp * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragColor    = inColor;
    fragNormal   = inNormal;
}