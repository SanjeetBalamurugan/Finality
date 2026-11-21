/**
 * @file
 * @ingroup render
 * @brief Collects scene render packets through the engine rendering facade.
 *
 * Renderer connects scene entities to a RenderDevice and maintains the render queue and active camera. Per-view visibility and backend recording are separate from authored scene components.
 *
 * @par Usage
 * Initialize with a live device, begin scene submission, push entities and end submission. Keep device lifetime longer than renderer use and release renderer resources before device shutdown.
 */
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
    /**
     * @ingroup render
     * @brief Collects scene render packets through the engine rendering facade.
     *
     * Renderer connects scene entities to a RenderDevice and maintains the render queue and active camera. Per-view visibility and backend recording are separate from authored scene components.
     *
     * @par Usage
     * Initialize with a live device, begin scene submission, push entities and end submission. Keep device lifetime longer than renderer use and release renderer resources before device shutdown.
     */
    class FAPI Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        /**
         * @brief Connects the rendering facade to a graphics backend.
         * @param device Live graphics device that outlives renderer use.
         * The pointer is non-owning. Shut down the renderer before destroying its device.
         */
        static void Initialize(RenderDevice* device);
        static void Shutdown();

        static void BeginScene();
        static void EndScene();

        static void PushEntity(Entity entity);
        static void PushEntity(Entity entity, const glm::mat4& worldTransform);
        static void ConfigureLODGroups(const Scene& scene);
        static RenderDevice* GetDevice() { return s_Device; }

        static void SetActiveCamera(Camera* camera) { s_ActiveCamera = camera; }
        static Camera* GetActiveCamera() { return s_ActiveCamera; }

        static void SetPostProcessPipeline(std::shared_ptr<Pipeline> pipeline);
        static std::shared_ptr<Pipeline> CreatePostProcessPipeline(const PipelineConfig& config);
        
        static uint32_t GetSubmittedCount() { return (uint32_t)s_RenderQueue.size(); }
        static uint32_t GetCulledCount() { return s_CulledCount; }
        static const std::vector<RenderPacket>& GetRenderQueue() { return s_RenderQueue; }

        static void SetDirectRenderingEnabled(bool enabled) { s_DirectRenderingEnabled = enabled; }
        static bool IsDirectRenderingEnabled() { return s_DirectRenderingEnabled; }

        static void SetFrustumCullingEnabled(bool enabled) { s_FrustumCullingEnabled = enabled; }

    private:
        static RenderDevice* s_Device;
        static std::vector<RenderPacket> s_RenderQueue;
        static inline Camera* s_ActiveCamera = nullptr;

        static Frustum s_CurrentFrustum;
        static bool s_FrustumCullingEnabled;

        static uint32_t s_CulledCount;
        static inline bool s_DirectRenderingEnabled = true;
        static inline ViewVisibilityState s_VisibilityState;

    protected:
        static bool TryGetCullingFrustum(const Frustum*& outFrustum);
        static bool IsFrustumCullingEnabled() { return s_FrustumCullingEnabled; }
    };
}
