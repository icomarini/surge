#version 450

// input ========================================
layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inPosition;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    vec4 color;
    uint isLight;
    // uint fragmentStageFlag;
};

layout(set = 0, binding = 0) uniform Scene
{
    mat4 projection;
    mat4 view;
    vec4 lightColor;
    vec3 lightPosition;
};

layout(set = 1, binding = 0) uniform sampler2D texSampler;

// output =======================================
layout(location = 0) out vec4 outColor;

void main()
{
    // if (fragmentStageFlag == 0)
    // {
    //     outColor = texture(texSampler, fragTexCoord);
    // }
    // else if (fragmentStageFlag == 1)
    // {
    //     outColor = vec4(fragColor, 1.0);
    // }
    // else if (fragmentStageFlag == 2)
    // {
    //     outColor = vec4(fragNormal, 1.0);
    // }
    // else
    // {
    //     outColor = vec4(1.0, 1.0, 1.0, 1.0);
    // }
    if (isLight == 1)
    {
        outColor = vec4(inColor, 1.0);
    }
    else
    {
        float ambientStrength = 0.1;
        vec3  ambient         = ambientStrength * lightColor.rgb;

        vec3 norm     = normalize(inNormal);
        vec3 lightDir = normalize(lightPosition - inPosition);
        vec3 diffuse  = max(dot(norm, lightDir), 0.0) * lightColor.rgb;
        outColor      = vec4((ambient + diffuse) * color.rgb, 1.0);
        // outColor      = lightColor * color;
    }
}
