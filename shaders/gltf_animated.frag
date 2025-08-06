#version 450

// input ========================================
layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inViewVec;
layout(location = 4) in vec3 inLightVec;

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
        // outColor = texture(texSampler, fragTexCoord);
        vec4 color    = texture(texSampler, inTexCoord) * vec4(inColor, 1.0);
        vec3 N        = normalize(inNormal);
        vec3 L        = normalize(inLightVec);
        vec3 V        = normalize(inViewVec);
        vec3 R        = reflect(-L, N);
        vec3 diffuse  = max(dot(N, L), 0.5) * inColor;
        vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
        outColor      = vec4(diffuse * color.rgb + specular, 1.0);
        // outColor      = texture(texSampler, inTexCoord);
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
