#version 450

// input ========================================
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    vec4 baseColorFactor;
    uint vertexStageFlag;
    uint fragmentStageFlag;
};

layout(set = 1, binding = 0) uniform sampler2D texSampler;

// output =======================================
layout(location = 0) out vec4 outColor;


void main()
{
    if (fragmentStageFlag == 0)
    {
        outColor = texture(texSampler, inTexCoord);
    }
    else if (fragmentStageFlag == 1)
    {
        outColor = vec4(inColor, 1.0);
    }
    else if (fragmentStageFlag == 2)
    {
        outColor = vec4(inNormal, 1.0);
    }
    else
    {
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
}