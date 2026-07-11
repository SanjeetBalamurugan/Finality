#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform UserMaterialData {
    layout(offset = 64) float time;
} material;

void main() {
    // Generate a pulsing sine wave effect over time
    float pulse = sin(material.time * 2.0) * 0.5 + 0.5;
    
    // Mix the vertex colors with the runtime uniform wave
    outColor = vec4(fragColor * pulse, 1.0);
}
