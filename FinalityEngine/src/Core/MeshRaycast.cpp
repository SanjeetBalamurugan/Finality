#include "Mesh.h"
#include <cmath>

bool FINALITY::Mesh::Raycast(const glm::vec3& origin, const glm::vec3& direction, const glm::mat4& world, float& distance) const
{
    if (m_Vertices.empty() || std::abs(glm::determinant(world)) < 1.0e-8f)
    {
        return false;
    }
    const auto inverse = glm::inverse(world);
    const auto rayOrigin = glm::vec3(inverse * glm::vec4(origin, 1));
    const auto rayDirection = glm::vec3(inverse * glm::vec4(direction, 0));
    const auto offsetToBounds = rayOrigin - m_BoundsCenter;
    const float lengthSquared = glm::dot(rayDirection, rayDirection);
    const float projection = glm::dot(offsetToBounds, rayDirection);
    const float radiusDistance = glm::dot(offsetToBounds, offsetToBounds) - m_BoundsRadius * m_BoundsRadius;
    const float discriminant = projection * projection - lengthSquared * radiusDistance;
    if (lengthSquared < 1.0e-12f || discriminant < 0 || (radiusDistance > 0 && projection > 0))
    {
        return false;
    }
    if ((-projection - std::sqrt(discriminant)) / lengthSquared > distance)
    {
        return false;
    }
    bool hit = false;
    const auto count = m_Indices.empty() ? m_Vertices.size() : m_Indices.size();
    for (size_t index = 0; index + 2 < count; index += 3)
    {
        const auto a = m_Indices.empty() ? index : m_Indices[index];
        const auto b = m_Indices.empty() ? index + 1 : m_Indices[index + 1];
        const auto c = m_Indices.empty() ? index + 2 : m_Indices[index + 2];
        if (a >= m_Vertices.size() || b >= m_Vertices.size() || c >= m_Vertices.size())
        {
            continue;
        }
        const auto edge1 = m_Vertices[b].Position - m_Vertices[a].Position;
        const auto edge2 = m_Vertices[c].Position - m_Vertices[a].Position;
        const auto perpendicular = glm::cross(rayDirection, edge2);
        const float determinant = glm::dot(edge1, perpendicular);
        if (std::abs(determinant) < 1.0e-8f)
        {
            continue;
        }
        const float inverseDeterminant = 1.0f / determinant;
        const auto offset = rayOrigin - m_Vertices[a].Position;
        const float u = glm::dot(offset, perpendicular) * inverseDeterminant;
        if (u < 0 || u > 1)
        {
            continue;
        }
        const auto cross = glm::cross(offset, edge1);
        const float v = glm::dot(rayDirection, cross) * inverseDeterminant;
        if (v < 0 || u + v > 1)
        {
            continue;
        }
        const float current = glm::dot(edge2, cross) * inverseDeterminant;
        if (current >= 0 && current < distance)
        {
            distance = current;
            hit = true;
        }
    }
    return hit;
}
