#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in mat4 instanceTransform;
layout(location = 0) out vec3 vertexColor;
layout(location = 1) out vec2 textureCoordinate;

layout(set = 0, binding = 0) uniform GlobalScene
{
    mat4 view;
    mat4 projection;
}

scene;

void main()
{
    gl_Position = scene.projection * scene.view * instanceTransform * vec4(position, 1);
    vertexColor = color;
    textureCoordinate = uv;
}
