#pragma once

#include <Core/Core.h>
#include <Core/RenderDevice.h>
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include <vector>
#include "Camera.h"

#include <Core/Pipeline.h>
#include "Camera/Frustum.h"

namespace FINALITY
{
    class FAPI Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        static void Initialize(RenderDevice* device);
        static void Shutdown();

        static void BeginScene();
        static void EndScene();

        static void PushEntity(Entity entity);
        static RenderDevice* GetDevice() { return s_Device; }

        static void SetActiveCamera(Camera* camera) { s_ActiveCamera = camera; }
        static Camera* GetActiveCamera() { return s_ActiveCamera; }

        static void SetPostProcessPipeline(std::shared_ptr<Pipeline> pipeline);
        static std::shared_ptr<Pipeline> CreatePostProcessPipeline(const PipelineConfig& config);
        
        static uint32_t GetSubmittedCount() { return (uint32_t)s_RenderQueue.size(); }
        static uint32_t GetCulledCount() { return s_CulledCount; }

    private:
        static RenderDevice* s_Device;
        static std::vector<RenderPacket> s_RenderQueue;
        static inline Camera* s_ActiveCamera = nullptr;

        static Frustum s_CurrentFrustum;
        static bool s_FrustumCullingEnabled;

        static uint32_t s_CulledCount;

    protected:
        static bool TryGetCullingFrustum(const Frustum*& outFrustum);
        static void SetFrustumCullingEnabled(bool enabled) { s_FrustumCullingEnabled = enabled; }
        static bool IsFrustumCullingEnabled() { return s_FrustumCullingEnabled; }
    };
}
