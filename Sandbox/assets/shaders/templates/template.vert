#version 450

// Binding 0: Per-Vertex Attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

// Binding 1: Per-Instance Attributes
layout(location = 3) in mat4 inInstanceTransform; // Consumes locations 3, 4, 5, 6
layout(location = 7) in vec4 inCustomData0;
layout(location = 8) in vec4 inCustomData1;
layout(location = 9) in vec4 inCustomData2;
layout(location = 10) in vec4 inCustomData3;

// Outputs to Fragment Shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragCustomData;

// Global Descriptor Set (Set 0)
layout(set = 0, binding = 0) uniform GlobalSceneUBO {
    mat4 view;
    mat4 proj;
} global;

// Push Constants
layout(push_constant) uniform PushConstants {
    float time;
} push;

void main() {
    vec4 worldPosition = inInstanceTransform * vec4(inPosition, 1.0);
    gl_Position = global.proj * global.view * worldPosition;

    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragCustomData = inCustomData0;
}