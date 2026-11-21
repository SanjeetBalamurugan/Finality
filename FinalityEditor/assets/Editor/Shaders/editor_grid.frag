#version 450

layout(location = 0) in vec3 worldPosition;
layout(location = 1) flat in vec3 cameraPosition;
layout(location = 0) out vec4 color;
layout(push_constant) uniform GridSettings
{
    layout(offset = 64) float spacing;
    layout(offset = 68) float fadeDistance;
} settings;

float gridLine(vec2 position, float spacing)
{
    vec2 coordinate = position / spacing;
    vec2 width = max(fwidth(coordinate), vec2(0.0001));
    vec2 line = abs(fract(coordinate - 0.5) - 0.5) / width;
    vec2 coverage = (1.0-min(line,vec2(1.0))) * (1.0-smoothstep(vec2(0.25),vec2(1.0),width));
    return max(coverage.x,coverage.y);
}

void main()
{
    float distanceToCamera = length(worldPosition - cameraPosition);
    float fade = 1.0 - smoothstep(settings.fadeDistance * 0.2, settings.fadeDistance, distanceToCamera);
    float minor = gridLine(worldPosition.xz, settings.spacing);
    float major = gridLine(worldPosition.xz, settings.spacing * 10.0);
    vec2 width = max(fwidth(worldPosition.xz), vec2(0.001));
    float xAxis = 1.0 - smoothstep(width.y, width.y * 2.0, abs(worldPosition.z));
    float zAxis = 1.0 - smoothstep(width.x, width.x * 2.0, abs(worldPosition.x));
    vec3 tint = mix(vec3(0.43, 0.48, 0.54), vec3(0.64, 0.68, 0.74), major);
    tint = mix(tint, vec3(0.75, 0.29, 0.29), xAxis);
    tint = mix(tint, vec3(0.27, 0.48, 0.78), zAxis);
    float alpha = max(max(minor * 0.3, major * 0.5), max(xAxis, zAxis) * 0.75) * fade;
    if (alpha < 0.01) discard;
    color = vec4(tint, alpha);
}
