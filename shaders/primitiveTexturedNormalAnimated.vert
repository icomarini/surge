#version 450

// input ========================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inJointIndices;
layout(location = 4) in vec4 inJointWeights;

layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(set = 0, binding = 0) uniform Scene {
    mat4 projection;
    mat4 view;
    vec4 lightColor;
    vec3 lightPosition;
};

layout(set = 2, binding = 0) readonly buffer JointMatrices {
    mat4 jointMatrices[];
};

// output =======================================
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outPosition;
layout(location = 2) out vec2 outTexCoord;


void main(void) {
    mat4 inverseModel = inverse(model);
    mat4 skin         = inJointWeights.x * jointMatrices[int(inJointIndices.x)] +
                inJointWeights.y * jointMatrices[int(inJointIndices.y)] +
                inJointWeights.z * jointMatrices[int(inJointIndices.z)] +
                inJointWeights.w * jointMatrices[int(inJointIndices.w)];
    gl_Position = vec4(inPosition, 1.0) * skin * model * view * projection;
    outPosition = vec3(vec4(inPosition, 1.0) * model);
    outNormal   = mat3(inverse(model)) * inNormal;
    outTexCoord = inTexCoord;

    // outNormal   = (vec4(mat3(transpose(inverse(model))) * inNormal, 1.0) * view * projection).xyz;
    // outTexCoord = inTexCoord;
    // vec4 pos    = vec4(inPosition, 1.0) * view;
    // outLightVec = (vec4(lightPosition, 1.0) * view).xyz;
    // outLightVec = lightPosition.xyz * mat3(view) - pos.xyz;
    // outViewVec = -pos.xyz;
}