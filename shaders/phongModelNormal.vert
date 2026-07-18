#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;


layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(set = 0, binding = 0) uniform Scene {
    mat4 projection;
    mat4 view;
    vec4 lightColor;
    vec3 lightPosition;
};

// output =======================================
layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outLightPosition;
layout(location = 3) out vec2 outTexCoord;
layout(location = 4) out mat3 outTBN;

void main(void) {
    gl_Position      = vec4(inPosition, 1.0) * model * view * projection;
    outPosition      = vec3(vec4(inPosition, 1.0) * model * view);
    outNormal        = mat3(inverse(model * view)) * inNormal;
    outLightPosition = vec3(vec4(lightPosition, 1.0) * view);
    outTexCoord      = inTexCoord;

    // Calculate Bitangent
    vec3 N = normalize(mat3(inverse(model * view)) * inNormal);
    vec3 T = normalize(mat3(inverse(model * view)) * inTangent.xyz);
    T      = normalize(T - dot(T, N) * N);  // Re-orthogonalize T
    vec3 B = cross(N, T) * inTangent.w;
    outTBN = mat3(T, B, N);
}