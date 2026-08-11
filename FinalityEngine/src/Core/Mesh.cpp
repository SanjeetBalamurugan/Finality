#include "Core/Mesh.h"
#include "Core/Application.h"
#include "Renderer/Renderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <glm/gtx/norm.hpp>

namespace FINALITY
{
    std::shared_ptr<Mesh> Mesh::Create(const std::vector<Vertex>& vertices)
    {
        switch (Application::GetActiveAPI())
        {
        case RendererAPI::VULKAN:
            return Renderer::GetDevice()->CreateMesh(vertices);
        default:
            return nullptr;
        }
    }

    std::shared_ptr<Mesh> Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        switch (Application::GetActiveAPI())
        {
        case RendererAPI::VULKAN:
            return Renderer::GetDevice()->CreateMesh(vertices, indices);
        default:
            return nullptr;
        }
    }
    void Mesh::ComputeBounds(const std::vector<Vertex>& vertices)
    {
        if (vertices.empty())
        {
            m_BoundsCenter = glm::vec3(0.0f);
            m_BoundsRadius = 0.0f;
            return;
        }

        glm::vec3 minB(FLT_MAX), maxB(-FLT_MAX);
        for (const auto& v : vertices)
        {
            minB = glm::min(minB, v.Position);
            maxB = glm::max(maxB, v.Position);
        }

        m_BoundsCenter = (minB + maxB) * 0.5f;

        float radiusSq = 0.0f;
        for (const auto& v : vertices)
        {
            radiusSq = std::max(radiusSq, glm::length2(v.Position - m_BoundsCenter));
        }
        m_BoundsRadius = std::sqrt(radiusSq);
    }
}
