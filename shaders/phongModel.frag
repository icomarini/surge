#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inLightPosition;
layout(location = 3) in vec2 inTexCoord;

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

layout(set = 1, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 1, binding = 1) uniform sampler2D specularSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;

// output =======================================
layout(location = 0) out vec4 outFragColor;

void main(void) {
    // ambient
    float ambientStrength = 0.01;
    vec3  ambientColor    = vec3(1.0, 1.0, 1.0);
    vec3  ambient         = ambientStrength * lightColor.rgb;

    // diffuse
    vec3  normal         = normalize(inNormal);
    vec3  lightDirection = normalize(inLightPosition - inPosition);
    float diffuseCoef    = max(dot(normal, lightDirection), 0.0);
    vec3  diffuse        = diffuseCoef * lightColor.rgb;

    // specular
    float specularStrength = 0.5;
    vec3  viewDirection    = normalize(-inPosition);
    vec3  reflectDirection = reflect(-lightDirection, normal);
    float specularCoef     = specularStrength * pow(max(dot(viewDirection, reflectDirection), 0.0), 256);
    vec3  specular         = specularCoef * lightColor.rgb * texture(specularSampler, inTexCoord).rgb;

    outFragColor = vec4(ambient + diffuse + specular, 1.0) * texture(diffuseSampler, inTexCoord);
}