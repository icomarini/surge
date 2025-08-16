#version 450

// input ========================================
layout(push_constant) uniform PushConstants
{
    vec3 a;
    vec3 b;
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
    vec3 vertex = vec3(0.0, 0.0, 0.0);
    if (gl_VertexIndex == 0)
    {
        vertex = a;
    }
    else if (gl_VertexIndex == 1)
    {
        vertex = b;
    }

    gl_Position = vec4(vertex, 1.0) * view * projection;
    outColor    = color;
}