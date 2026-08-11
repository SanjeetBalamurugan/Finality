#include <Core/Core.h>

#include "Frustum.h"

FINALITY::Frustum FINALITY::Frustum::FromViewProjection(const glm::mat4& viewProjection)
{
    Frustum frustum;
    glm::mat4 m = glm::transpose(viewProjection);

    // Left Plane
    frustum.m_Planes[0] = m[3] + m[0];
    // Right Plane
    frustum.m_Planes[1] = m[3] - m[0];
    // Bottom Plane
    frustum.m_Planes[2] = m[3] + m[1];
    // Top Plane
    frustum.m_Planes[3] = m[3] - m[1];
    // Near Plane (Vulkan 0 to 1 depth mapping — z row alone)
    frustum.m_Planes[4] = m[2];
    // Far Plane
    frustum.m_Planes[5] = m[3] - m[2];

    for (int i = 0; i < 6; i++)
    {
        float length = glm::length(glm::vec3(frustum.m_Planes[i]));
        if (length > 0.0001f)
            frustum.m_Planes[i] /= length;
    }

    return frustum;
}

bool FINALITY::Frustum::IntersectsSphere(const glm::vec3& center, float radius) const
{
    for (int i = 0; i < 6; i++)
    {
        float distance = glm::dot(glm::vec3(m_Planes[i]), center) + m_Planes[i].w;
        if (distance < -radius)
            return false;
    }
    return true;
}

bool FINALITY::Frustum::IntersectsAABB(const glm::vec3& min, const glm::vec3& max) const
{
    for (int i = 0; i < 6; i++)
    {
        const glm::vec4& plane = m_Planes[i];
        glm::vec3 positiveVertex(
            plane.x >= 0.0f ? max.x : min.x,
            plane.y >= 0.0f ? max.y : min.y,
            plane.z >= 0.0f ? max.z : min.z
        );

        if (glm::dot(glm::vec3(plane), positiveVertex) + plane.w < 0.0f)
            return false;
    }
    return true;
}
