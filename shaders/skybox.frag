#version 450

// input ========================================
layout(location = 0) in vec3 inUVW;

layout(set = 1, binding = 0) uniform samplerCube samplerCubeMap;

// output =======================================
layout(location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = texture(samplerCubeMap, inUVW);
}