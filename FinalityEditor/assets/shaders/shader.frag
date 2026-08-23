#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(push_constant) uniform UserMaterialData {
    layout(offset = 64) float time;
} material;

void main() {
    vec4 sampledColor = texture(textureSampler, fragTexCoord);
    
    float pulse = sin(material.time * 2.0) * 0.2 + 0.8;
    
    outColor = sampledColor * vec4(fragColor, 1.0) * pulse;
}
