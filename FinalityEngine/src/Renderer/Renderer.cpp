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
        s_ActiveCamera = nullptr;
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

        const auto& mat = entity.GetComponent<MaterialComponent>();
        if (mat.MaterialInstance)
        {
            packet.PipelineInstance = mat.MaterialInstance->GetPipeline();
            packet.CustomPushData = mat.MaterialInstance->GetRawDataBuffer();

            packet.Textures = mat.MaterialInstance->GetTextures();
        }

        if (entity.HasComponent<TransformComponent>())
        {
            const auto& transform = entity.GetComponent<TransformComponent>();
            packet.Transform = transform.GetTransformMatrix();
        }
        else
        {
            packet.Transform = glm::mat4(1.0f);
        }

        s_RenderQueue.push_back(packet);
    }

    void Renderer::EndScene()
    {
        if (!s_Device) return;

        s_Device->DrawQueue(s_RenderQueue);
    }
}
