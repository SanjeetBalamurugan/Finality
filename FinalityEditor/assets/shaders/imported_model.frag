#version 450

layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 textureCoordinate;
layout(location = 0) out vec4 color;
layout(set = 1, binding = 0) uniform sampler2D textureSampler;
layout(push_constant) uniform ImportedMaterial
{
    layout(offset = 64) vec4 baseColor;
} material;

void main()
{
    vec4 sampled = texture(textureSampler,textureCoordinate);
    vec3 linearColor = mix(sampled.rgb / 12.92, pow((sampled.rgb + 0.055) / 1.055,vec3(2.4)),step(vec3(0.04045),sampled.rgb));
    color = vec4(linearColor,sampled.a) * material.baseColor * vec4(vertexColor,1);
}
