#version 450

layout(location = 0) in vec3 inPos;

layout(binding = 0) uniform UBO
{
    mat4 mvp;
}
ubo;

layout(location = 0) out vec3 outUVW;

void main()
{
    outUVW      = inPos;
    gl_Position = vec4(inPos.xyz, 1.0) * ubo.mvp;
}
