#version 450

// input ========================================
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;
// layout(location = 2) in vec3 inViewVec;
// layout(location = 3) in vec3 inLightVec;

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

layout(set = 1, binding = 0) uniform sampler2D texSampler;

// output =======================================
layout(location = 0) out vec4 outFragColor;

void main(void) {
    // outFragColor = texture(texSampler, inTexCoord);
    vec3 normal = normalize(inNormal);
    vec3 light  = normalize(lightPosition - inPosition);

    outFragColor = max(dot(normal, light), 0.0) * texture(texSampler, inTexCoord);

    // vec3 N        = normalize(inNormal);
    // vec3 L        = normalize(inLightVec);
    // vec3 V        = normalize(inViewVec);
    // vec3 R        = reflect(-L, N);
    // vec3 diffuse  = max(dot(N, L), 0.0) * inColor;
    // vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
    // outFragColor  = vec4(diffuse * color.rgb + specular, 1.0);
}