#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants
{
    mat4 model;
};

layout(set = 0, binding = 0) uniform Scene
{
    mat4 projection;
    mat4 view;
};

// output =======================================
layout(location = 0) out vec3 outUVW;

void main()
{
    outUVW      = inPosition;
    gl_Position = vec4(inPosition, 1.0) * model;
}
