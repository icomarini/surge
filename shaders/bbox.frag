#version 450


layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    uint vertexStageFlag;
    uint fragmentStageFlag;
}
pushConstants;

void main()
{
    // if (pushConstants.fragmentStageFlag == 0)
    // {
    //     outColor = texture(texSampler, fragTexCoord);
    // }
    // else if (pushConstants.fragmentStageFlag == 1)
    // {
    //     outColor = vec4(fragColor, 1.0);
    // }
    // else if (pushConstants.fragmentStageFlag == 2)
    // {
    //     outColor = vec4(fragNormal, 1.0);
    // }
    // else
    // {
    //     outColor = vec4(1.0, 1.0, 1.0, 1.0);
    // }
    outColor = fragColor;
}
