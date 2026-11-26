#include "Renderer.h"
#include <stdexcept>
#include "ViewVisibility.h"

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
        s_VisibilityState.LODLevels.clear();
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
        glm::mat4 transform(1);
        if (entity.HasComponent<TransformComponent>())
        {
            transform = entity.GetComponent<TransformComponent>().GetTransformMatrix();
        }
        PushEntity(entity, transform);
    }

    void Renderer::PushEntity(Entity entity, const glm::mat4& worldTransform)
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

        RenderPacket packet{};
        packet.MeshData = meshComp.MeshData;
        packet.Occluder = meshComp.Occluder;
        packet.EntityID = entity.GetComponent<TagComponent>().uuid;

        const auto& mat = entity.GetComponent<MaterialComponent>();
        if (mat.MaterialInstance)
        {
            packet.PipelineInstance = mat.MaterialInstance->GetPipeline();
            packet.CustomPushData = mat.MaterialInstance->GetRawDataBuffer();
            packet.Textures = mat.MaterialInstance->GetTextures();
            packet.MaterialInstance = mat.MaterialInstance;
        }

        packet.Transform = worldTransform;

        s_RenderQueue.push_back(packet);
    }

    void Renderer::EndScene()
    {
        if (!s_Device)
        {
            return;
        }
        if (!s_DirectRenderingEnabled)
        {
            return;
        }

        if (s_ActiveCamera)
        {
            const RenderView view{ s_ActiveCamera->GetViewMatrix(), s_ActiveCamera->GetProjection(), s_ActiveCamera->GetOcclusionCulling() };
            s_Device->SetFrameView(view);
            RenderStatistics statistics;
            s_Device->DrawQueue(BuildViewQueue(s_RenderQueue, view, statistics, &s_VisibilityState));
        }
        else
        {
            s_Device->DrawQueue(s_RenderQueue);
        }
    }

    void Renderer::SetPostProcessPipeline(std::shared_ptr<Pipeline> pipeline)
    {
        if (!s_Device)
        {
            throw std::runtime_error("Renderer is not initialized");
        }
        s_Device->SetPostProcessPipeline(std::move(pipeline));
    }

    std::shared_ptr<Pipeline> Renderer::CreatePostProcessPipeline(const PipelineConfig& config)
    {
        if (!s_Device)
        {
            throw std::runtime_error("Renderer is not initialized");
        }
        return s_Device->CreatePostProcessPipeline(config);
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
} // namespace FINALITY
