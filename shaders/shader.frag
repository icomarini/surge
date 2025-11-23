#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoordinate;
layout(location = 3) in vec3 inLightPosition;
// layout(location = 2) in vec3 inColor;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    vec4 baseColor;
    uint isLight;
};

layout(set = 0, binding = 0) uniform Scene
{
    mat4 projection;
    mat4 view;
    vec4 lightColor;
    vec3 lightPosition;
};

layout(set = 1, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 1, binding = 1) uniform sampler2D specularSampler;

// output =======================================
layout(location = 0) out vec4 outColor;

void main()
{
    if (isLight == 1)
    {
        outColor = baseColor;
    }
    else
    {
        // ambient
        float ambientStrength = 0.001;
        vec4  ambient         = ambientStrength * lightColor;

        // diffuse
        vec3 normal         = normalize(inNormal);
        vec3 lightDirection = normalize(inLightPosition - inPosition);
        vec4 diffuse        = max(dot(normal, lightDirection), 0.0) * lightColor;

        // specular
        float specularStrength = 0.5;
        vec3  viewDirection    = normalize(-inPosition);
        vec3  reflectDirection = reflect(-lightDirection, normal);
        vec4  specular = specularStrength * pow(max(dot(viewDirection, reflectDirection), 0.0), 64) * lightColor *
                        texture(specularSampler, inTextureCoordinate);

        outColor = (ambient + diffuse) * texture(diffuseSampler, inTextureCoordinate) + specular;
        // outColor = baseColor;
    }
}