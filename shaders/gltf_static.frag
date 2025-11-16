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
        vec3 normal         = normalize(inNormal);
        vec3 lightDirection = normalize(lightPosition - inPosition);

        float ambientStrength = 0.1;
        vec3  ambient         = ambientStrength * lightColor.rgb;

        vec3 diffuse = max(dot(normal, lightDirection), 0.0) * lightColor.rgb;

        float specularStrength = 0.5;
        vec3  viewPosition     = vec3(view[0][3], view[1][3], view[2][3]);
        vec3  viewDirection    = normalize(inPosition - viewPosition);
        vec3  reflectDirection = reflect(-lightDirection, normal);
        vec3  specular = specularStrength * pow(max(dot(viewDirection, reflectDirection), 0.0), 32) * lightColor.rgb;

        outColor = vec4((ambient + diffuse + specular) * color.rgb, 1.0);
        // outColor      = lightColor * color;
    }
}
