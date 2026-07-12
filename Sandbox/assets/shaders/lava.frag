#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform LavaMaterialData {
    layout(offset = 64) float time;
    float speed;
    float scale;
    float glowIntensity;
} lava;

void main() {
    vec2 uv = fragTexCoord * lava.scale;
    float motion = sin(uv.x + lava.time * lava.speed) + cos(uv.y + lava.time * lava.speed);
    
    vec3 baseMagma = vec3(0.8, 0.2, 0.0);
    vec3 glowingCore = vec3(1.0, 0.6, 0.0);
    
    float blendFactor = smoothstep(-1.0, 1.0, motion);
    vec3 finalLavaColor = mix(baseMagma, glowingCore, blendFactor) * lava.glowIntensity;
    
    outColor = vec4(finalLavaColor, 1.0);
}
