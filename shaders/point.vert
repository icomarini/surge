#version 450

// input ========================================
layout(push_constant) uniform PushConstants
{
    vec3 p;
    vec4 color;
};

layout(set = 0, binding = 0) uniform Scene
{
    mat4 projection;
    mat4 view;
};

// output =======================================
layout(location = 0) out vec4 outColor;

void main()
{
    gl_Position  = vec4(p, 1.0) * view * projection;
    gl_PointSize = 20.0f;
    outColor     = color;
}