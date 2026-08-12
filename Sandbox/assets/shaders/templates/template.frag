#version 450

// Inputs from Vertex Shader (must match location and types)
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragCustomData;

// Output Attachment
layout(location = 0) out vec4 outColor;

// Material Descriptor Set (Set 1)
layout(set = 1, binding = 0) uniform sampler2D mainTexture;

void main() {
    vec4 texColor = texture(mainTexture, fragTexCoord);
    outColor = texColor * vec4(fragColor, 1.0);
}