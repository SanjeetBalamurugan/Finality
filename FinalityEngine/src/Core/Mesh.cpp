#include "Core/Mesh.h"
#include "Core/Application.h"
#include "Renderer/Renderer.h"

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
}
