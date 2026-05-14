#version 450

// input ========================================
layout(triangles) in;
layout(location = 0) in vec4 inPosition[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec4 inTangent[];

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
    uint isLight;
};

layout(set = 0, binding = 0) uniform Scene {
    mat4 projection;
    mat4 view;
    vec4 lightColor;
    vec3 lightPosition;
};

// output =======================================
layout(line_strip, max_vertices = 18) out;
layout(location = 0) out vec3 outColor;

// vec3 computeNormal()
// {
//     vec3 tangentNormal = texture(normalSampler, inTextureCoordinate).xyz * 2.0 - 1.0;

//     vec3 N   = normalize(inNormal);
//     vec3 T   = normalize(inTangent.xyz);
//     vec3 B   = normalize(cross(N, T));
//     mat3 TBN = mat3(T, B, N);
//     return normalize(TBN * tangentNormal);
// }

void main(void) {
    float normalLength = 0.1;
    for (int i = 0; i < gl_in.length(); i++) {
        vec3 position = gl_in[i].gl_Position.xyz;
        vec3 normal   = normalize(inNormal[i].xyz);

        // gl_Position = ubo.projection * (ubo.model * vec4(pos, 1.0));
        // gl_Position = vec4(position, 1.0) * model * view * projection;
        gl_Position = inPosition[i];
        outColor    = vec3(0.0, 0.0, 1.0);
        EmitVertex();

        // gl_Position = ubo.projection * (ubo.model * vec4(pos + normal * normalLength, 1.0));
        gl_Position = vec4(position + normal * normalLength, 1.0) * model * view * projection;
        // gl_Position = (vec4(position, 1.0) + vec4(normal * normalLength, 1.0)) * model * view * projection;
        // gl_Position = vec4(inPosition[i].xyz + vec3(vec4(normal * normalLength, 1.0) * model * view *
        // projection), 1.0);
        outColor = vec3(0.0, 0.0, 1.0);
        EmitVertex();

        EndPrimitive();
    }
    for (int i = 0; i < gl_in.length(); i++) {
        vec3 position = gl_in[i].gl_Position.xyz;
        vec3 tangent  = normalize(inTangent[i].xyz);

        // gl_Position = ubo.projection * (ubo.model * vec4(pos, 1.0));
        gl_Position = vec4(position, 1.0) * model * view * projection;
        outColor    = vec3(1.0, 0.0, 0.0);
        EmitVertex();

        // gl_Position = ubo.projection * (ubo.model * vec4(pos + normal * normalLength, 1.0));
        gl_Position = vec4(position + tangent * normalLength, 1.0) * model * view * projection;
        outColor    = vec3(1.0, 0.0, 0.0);
        EmitVertex();

        EndPrimitive();
    }
    for (int i = 0; i < gl_in.length(); i++) {
        vec3 position  = gl_in[i].gl_Position.xyz;
        vec3 N         = normalize(inNormal[i].xyz);
        vec3 T         = normalize(inTangent[i].xyz);
        vec3 bitangent = normalize(cross(N, T));

        // gl_Position = ubo.projection * (ubo.model * vec4(pos, 1.0));
        gl_Position = vec4(position, 1.0) * model * view * projection;
        outColor    = vec3(0.0, 1.0, 0.0);
        EmitVertex();

        // gl_Position = ubo.projection * (ubo.model * vec4(pos + normal * normalLength, 1.0));
        gl_Position = vec4(position + bitangent * normalLength, 1.0) * model * view * projection;
        outColor    = vec3(0.0, 1.0, 0.0);
        EmitVertex();

        EndPrimitive();
    }
}