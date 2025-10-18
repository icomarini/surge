#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants
{
    mat4 model;
};

// output =======================================
layout(location = 0) out vec3 outUVW;

void main()
{
    outUVW      = inPosition;
    gl_Position = vec4(inPosition.xyz, 1.0) * model;
}
