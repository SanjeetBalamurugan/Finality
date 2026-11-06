#include "Core/Mesh.h"
#include "Core/Application.h"
#include "Renderer/Renderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <cmath>
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
        std::vector<Vertex> prepared = vertices;
        CalculateNormalsAndTangents(prepared, indices);
        switch (Application::GetActiveAPI())
        {
        case RendererAPI::VULKAN:
            return Renderer::GetDevice()->CreateMesh(prepared, indices);
        default:
            return nullptr;
        }
    }

    std::shared_ptr<Mesh> Mesh::Create(const std::string& meshPath)
    {
        switch (Application::GetActiveAPI())
        {
        case RendererAPI::VULKAN:
            return Renderer::GetDevice()->CreateMesh(meshPath);
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

void FINALITY::Mesh::CalculateNormalsAndTangents(
    std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    if (vertices.empty() || indices.empty()) return;
    std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> tangents(vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(vertices.size(), glm::vec3(0.0f));

    for (size_t index = 0; index + 2 < indices.size(); index += 3)
    {
        const uint32_t a = indices[index], b = indices[index + 1], c = indices[index + 2];
        if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size()) continue;
        const glm::vec3 e1 = vertices[b].Position - vertices[a].Position;
        const glm::vec3 e2 = vertices[c].Position - vertices[a].Position;
        const glm::vec2 u1 = vertices[b].TexCoord - vertices[a].TexCoord;
        const glm::vec2 u2 = vertices[c].TexCoord - vertices[a].TexCoord;
        const glm::vec3 n = glm::cross(e1, e2);
        if (glm::dot(n, n) < 1.0e-12f) continue;
        for (auto i : {a, b, c}) normals[i] += n;

        const float det = u1.x * u2.y - u1.y * u2.x;
        if (std::abs(det) < 1.0e-8f) continue;
        const float inv = 1.0f / det;
        const glm::vec3 t = (e1 * u2.y - e2 * u1.y) * inv;
        const glm::vec3 bt = (e2 * u1.x - e1 * u2.x) * inv;
        for (auto i : {a, b, c})
        {
            tangents[i] += t;
            bitangents[i] += bt;
        }
    }

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        glm::vec3 n = normals[i];
        n = glm::dot(n, n) > 1.0e-12f ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 t = tangents[i] - n * glm::dot(n, tangents[i]);
        if (glm::dot(t, t) < 1.0e-12f)
        {
            const glm::vec3 axis = std::abs(n.z) < 0.9f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
            t = glm::cross(axis, n);
        }
        t = glm::normalize(t);
        vertices[i].Normal = n;
        vertices[i].Tangent = glm::vec4(t, glm::dot(glm::cross(n, t), bitangents[i]) < 0.0f ? -1.0f : 1.0f);
    }
}
