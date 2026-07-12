#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

void main() {
    vec4 sceneColor = texture(sceneTexture, fragTexCoord);

    vec2 uv = fragTexCoord * 2.0 - 1.0;

    float distance = length(uv);
    float vignette = smoothstep(0.3, 1.1, distance);
    vignette = 1.0 - vignette * 0.85;

    outColor = vec4(sceneColor.rgb * vignette, sceneColor.a);
}
