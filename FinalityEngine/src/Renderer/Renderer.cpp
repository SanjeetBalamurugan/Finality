#include "Renderer.h"
#include <Vulkan/VKRenderDevice.h>
#include <Vulkan/VKPipeline.h>

namespace FINALITY
{
    RenderDevice* Renderer::s_Device = nullptr;
    std::vector<RenderPacket> Renderer::s_RenderQueue;

    Frustum Renderer::s_CurrentFrustum;
    bool Renderer::s_FrustumCullingEnabled = true;

    uint32_t Renderer::s_CulledCount = 0;

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
        s_CulledCount = 0;

        if (s_ActiveCamera)
        {
            glm::mat4 projection = s_ActiveCamera->GetProjection();
            glm::mat4 view = s_ActiveCamera->GetViewMatrix();
            glm::mat4 viewProj = projection * view;

            s_CurrentFrustum = Frustum::FromViewProjection(viewProj);
        }
    }

    void Renderer::PushEntity(Entity entity)
    {
        if (!entity.HasComponent<MeshComponent>() || !entity.HasComponent<MaterialComponent>())
        {
            return;
        }

        const auto& meshComp = entity.GetComponent<MeshComponent>();
        if (!meshComp.MeshData)
        {
            return;
        }

        glm::mat4 transform = glm::mat4(1.0f);
        if (entity.HasComponent<TransformComponent>())
        {
            transform = entity.GetComponent<TransformComponent>().GetTransformMatrix();
        }

        const Frustum* cullingFrustum = nullptr;
        if (TryGetCullingFrustum(cullingFrustum))
        {
            glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(meshComp.MeshData->GetBoundsCenter(), 1.0f));

            glm::vec3 scale;
            scale.x = glm::length(glm::vec3(transform[0]));
            scale.y = glm::length(glm::vec3(transform[1]));
            scale.z = glm::length(glm::vec3(transform[2]));
            float maxScale = std::max({ scale.x, scale.y, scale.z });

            float worldRadius = meshComp.MeshData->GetBoundsRadius() * maxScale;

            if (!cullingFrustum->IntersectsSphere(worldCenter, worldRadius))
            {
                s_CulledCount++;
                return; // Outside the view frustum, skip entirely.
            }
        }

        RenderPacket packet{};
        packet.MeshData = meshComp.MeshData;

        const auto& mat = entity.GetComponent<MaterialComponent>();
        if (mat.MaterialInstance)
        {
            packet.PipelineInstance = mat.MaterialInstance->GetPipeline();
            packet.CustomPushData = mat.MaterialInstance->GetRawDataBuffer();
            packet.Textures = mat.MaterialInstance->GetTextures();
            packet.MaterialKey = mat.MaterialInstance.get();
        }

        packet.Transform = transform;

        s_RenderQueue.push_back(packet);
    }

    void Renderer::EndScene()
    {
        if (!s_Device) return;

        s_Device->DrawQueue(s_RenderQueue);
    }

    void Renderer::SetPostProcessPipeline(std::shared_ptr<Pipeline> pipeline)
    {
        switch (s_Device->GetActiveApi())
        {
        case RendererAPI::NONE:
        {
            throw std::runtime_error("Renderer API is currently set to None!");
            break;
        }
        case RendererAPI::VULKAN:
        {
            auto* vkDevice = static_cast<VKRenderDevice*>(GetDevice());
            if (!vkDevice)
            {
                throw std::runtime_error("Vulkan Render Device is invalid during post-process setup!");
            }

            vkDevice->SetPostProcessPipeline(pipeline);
            break;
        }
        default:
        {
            throw std::runtime_error("Unknown or unsupported graphics API configuration layer!");
            break;
        }
        }
    }

    std::shared_ptr<Pipeline> Renderer::CreatePostProcessPipeline(const PipelineConfig& config)
    {
        switch (s_Device->GetActiveApi())
        {
        case RendererAPI::NONE:
        {
            throw std::runtime_error("Renderer API is None during post-process pipeline compilation!");
            return nullptr;
        }
        case RendererAPI::VULKAN:
        {
            auto* vkDevice = static_cast<VKRenderDevice*>(GetDevice());
            if (!vkDevice)
            {
                throw std::runtime_error("Vulkan Render Device is invalid during post-process pipeline compilation!");
            }

            return std::make_shared<VKPipeline>(
                vkDevice->GetActiveDevice(),
                vkDevice->GetSwapChain()->GetVKRenderPass(),
                config,
                vkDevice->GetPostProcessDescriptorSetLayout()
            );
        }
        default:
        {
            throw std::runtime_error("Unknown or unsupported graphics API configuration layer!");
            return nullptr;
        }
        }
    }
    bool Renderer::TryGetCullingFrustum(const Frustum*& outFrustum)
    {
        if (!s_FrustumCullingEnabled || !s_ActiveCamera)
        {
            outFrustum = nullptr;
            return false;
        }

        outFrustum = &s_CurrentFrustum;
        return true;
    }
}
