#version 450

layout(location = 0) in vec3 position;
layout(location = 3) in mat4 instanceTransform;
layout(location = 0) out vec3 worldPosition;
layout(location = 1) flat out vec3 cameraPosition;
layout(set = 0, binding = 0) uniform GlobalScene
{
    mat4 view;
    mat4 projection;
} scene;

void main()
{
    worldPosition = vec3(instanceTransform * vec4(position, 1.0));
    cameraPosition = vec3(inverse(scene.view)[3]);
    gl_Position = scene.projection * scene.view * vec4(worldPosition, 1.0);
}
