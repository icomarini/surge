#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(binding = 0) uniform UBO
{
    mat4  vp;
    mat4  depthMVP;
    mat4  model;
    vec4  lightPos;
    float zNear;
    float zFar;
}
ubo;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outViewVec;
layout(location = 3) out vec3 outLightVec;
layout(location = 4) out vec4 outShadowCoord;
layout(location = 5) out vec2 outUV;

const mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 1.0);

void main()
{
    outColor  = inColor;
    outNormal = inNormal;
    outUV     = inUV;

    gl_Position = ubo.vp * ubo.model * vec4(inPos.xyz, 1.0);

    vec4 pos    = ubo.model * vec4(inPos, 1.0);
    outNormal   = mat3(ubo.model) * inNormal;
    outLightVec = normalize(ubo.lightPos.xyz - inPos);
    outViewVec  = -pos.xyz;

    outShadowCoord = (biasMat * ubo.depthMVP * ubo.model) * vec4(inPos, 1.0);
}
