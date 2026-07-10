#include "Renderer.h"

namespace FINALITY
{
    RenderDevice* Renderer::s_Device = nullptr;
    std::vector<RenderPacket> Renderer::s_RenderQueue;

    void Renderer::Initialize(RenderDevice* device)
    {
        s_Device = device;
    }

    void Renderer::Shutdown()
    {
        s_RenderQueue.clear();
        s_Device = nullptr;
    }

    void Renderer::BeginScene()
    {
        s_RenderQueue.clear();
    }

    void Renderer::PushEntity(Entity entity)
    {
        if (!entity.HasComponent<MeshComponent>() || !entity.HasComponent<MaterialComponent>())
        {
            return;
        }

        RenderPacket packet{};
        packet.MeshData = entity.GetComponent<MeshComponent>().MeshData;
        packet.PipelineInstance = entity.GetComponent<MaterialComponent>().PipelineInstance;

        if (entity.HasComponent<TransformComponent>())
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            packet.Position = transform.Position;
            packet.Rotation = transform.Rotation;
            packet.Scale = transform.Scale;
        }
        else
        {
            packet.Position = glm::vec3(0.0f);
            packet.Rotation = glm::vec3(0.0f);
            packet.Scale = glm::vec3(1.0f);
        }

        s_RenderQueue.push_back(packet);
    }

    void Renderer::EndScene()
    {
        if (!s_Device) return;

        s_Device->DrawQueue(s_RenderQueue);
    }
}
