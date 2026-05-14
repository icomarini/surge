#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoordinate;
layout(location = 3) in vec3 inLightPosition;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in mat3 inTBN;
// layout(location = 2) in vec3 inColor;

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
layout(location = 0) out vec4 outColor;

vec3 computeNormal() {
    vec3 tangentNormal = normalize(texture(normalSampler, inTextureCoordinate).rgb * 2.0 - 1.0);
    // tangentNormal      = vec3(tangentNormal.x, -tangentNormal.y, tangentNormal.z);
    // // vec3 tangentNormal = texture(normalSampler, inTextureCoordinate).rgb;

    // vec3 T = normalize(inTangent);
    // vec3 N = normalize(inNormal);
    // vec3 B = normalize(cross(N, T));
    // return normalize(mat3(T, B, N) * normalize(tangentNormal));
    // return normalize(inNormal);
    return normalize(inTBN * tangentNormal);
}

void main() {
    if (isLight == 1) {
        outColor = baseColor;
    } else {
        // ambient
        float ambientStrength = 0.001;
        vec4  ambient         = ambientStrength * lightColor;

        // diffuse
        // vec3 normal = normalize(inNormal);
        // vec3 normal = normalize(2.0 * texture(normalSampler, inTextureCoordinate).rgb - 1.0);
        vec3 normal         = computeNormal();
        vec3 lightDirection = normalize(inLightPosition - inPosition);
        vec4 diffuse        = max(dot(normal, lightDirection), 0.0) * lightColor;

        // specular
        float specularStrength = 0.5;
        vec3  viewDirection    = normalize(-inPosition);
        vec3  reflectDirection = reflect(-lightDirection, normal);
        vec4  specular = specularStrength * pow(max(dot(viewDirection, reflectDirection), 0.0), 64) * lightColor *
                        texture(specularSampler, inTextureCoordinate).r;

        outColor = (ambient + diffuse) * texture(diffuseSampler, inTextureCoordinate) + specular;
        // outColor = baseColor;
    }
}