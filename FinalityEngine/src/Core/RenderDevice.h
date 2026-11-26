/**
 * @file
 * @ingroup core
 * @brief Defines the graphics backend boundary and render submission contract.
 *
 * The device exposes frame operations, resource creation, drawing and deferred resource retirement. RenderPacket carries geometry, pipeline, transform, material, texture and visibility inputs without exposing Vulkan command types to scene code.
 *
 * @par Usage
 * Keep backend-specific synchronization inside the implementation. Retire resources after submitted work completes; WaitIdle is an explicit synchronization operation, not routine per-object cleanup.
 */
#pragma once
#include <functional>
#include "Core.h"
#include "Window.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "Texture.h"
#include "RenderTypes.h"
#include "RenderTarget.h"
#include "RenderView.h"
#include <Renderer/LODSelection.h>

namespace FINALITY
{
    class Mesh;
    class Pipeline;
    class Shader;
    class Material;
    struct PipelineConfig;
    struct Vertex;

    /** @brief Scene draw submission retaining geometry, material and visibility inputs until backend recording. */
    struct RenderPacket
    {
        std::shared_ptr<Mesh> MeshData;
        std::shared_ptr<Pipeline> PipelineInstance;
        glm::mat4 Transform{ 1.0f };

        std::vector<uint8_t> CustomPushData;
        std::unordered_map<std::string, std::shared_ptr<Texture>> Textures;
        std::shared_ptr<const Material> MaterialInstance;
        bool EditorOverlay = false;
        bool Occluder = false;
        uint64_t EntityID = 0;
        std::shared_ptr<const RenderLODGroup> LODGroup;
        int LODLevel = 0;
    };

    /** @brief Per-instance transform and fixed-size custom data uploaded to the graphics backend. */
    struct InstancePayload
    {
        glm::mat4 Transform;
        uint8_t CustomData[64];
    };

    /**
	 * @ingroup core
	 * @brief Defines the graphics backend boundary and render submission contract.
	 *
	 * The device exposes frame operations, resource creation, drawing and deferred resource retirement. RenderPacket carries geometry, pipeline, transform, material, texture and visibility inputs without exposing Vulkan command types to scene code.
	 *
	 * @par Usage
	 * Keep backend-specific synchronization inside the implementation. Retire resources after submitted work completes; WaitIdle is an explicit synchronization operation, not routine per-object cleanup.
	 */
    class FAPI RenderDevice
    {
    public:
        virtual ~RenderDevice() = default;

        virtual void Initialize(const NativeWindowHandle& handle) = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void PresentFrame() = 0;

        /**
		 * @brief Waits for outstanding device work to finish.
		 * This is an explicit synchronization boundary. Routine object cleanup should use
		 * deferred retirement to avoid stalling the entire device.
		 */
        virtual void WaitIdle() = 0;
        /**
         * @brief Defers resource cleanup until backend frame work permits its release.
         * @param release Cleanup operation retaining the resources it needs to release.
         * Use retirement when replacing resources that recorded or submitted work may still
         * reference. The backend controls completion and draining of cleanup operations.
         */
        virtual void RetireResource(std::function<void()> release) = 0;
        virtual void FlushRetiredResources() = 0;
        virtual void Clear(float r, float g, float b, float a) = 0;

        virtual void SetWindowSpec(const WindowSpec& spec) = 0;
        virtual WindowSpec GetWindowSpec() = 0;

        virtual void DrawQueue(const std::vector<RenderPacket>& queue) = 0;
        virtual RendererAPI GetActiveApi() const = 0;

        virtual std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices) = 0;
        virtual std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) = 0;
        virtual std::shared_ptr<Mesh> CreateMesh(const std::string& meshPath) = 0;
        virtual std::shared_ptr<Pipeline> CreatePipeline(const PipelineConfig& config) = 0;
        virtual std::unique_ptr<Shader> CreateShader(const std::string& path) = 0;
        virtual std::shared_ptr<Pipeline> CreatePostProcessPipeline(const PipelineConfig& config) = 0;
        virtual void SetPostProcessPipeline(std::shared_ptr<Pipeline> pipeline) = 0;
        virtual void SetFrameView(const RenderView& view) = 0;

        virtual std::shared_ptr<RenderTarget> CreateRenderTarget(uint32_t width, uint32_t height) = 0;
        virtual void ResizeRenderTarget(const std::shared_ptr<RenderTarget>& target, uint32_t width, uint32_t height) = 0;

        void UpdateRenderTargetCamera(const std::shared_ptr<RenderTarget>& target, Camera* camera)
        {
            if (target && camera)
            {
                SetRenderTargetView(target, { camera->GetViewMatrix(), camera->GetProjection() });
            }
        }

        virtual void SetRenderTargetView(const std::shared_ptr<RenderTarget>& target, const RenderView& view) = 0;
        virtual void DestroyRenderTargetGpuResources(RenderTarget& target) = 0;
        virtual void RenderSceneToTarget(const std::shared_ptr<RenderTarget>& target, const std::vector<RenderPacket>& queue) = 0;
        virtual void* GetRenderTargetTextureHandle(const std::shared_ptr<RenderTarget>& target) = 0;
        virtual void SetRenderTargetClearColor(const std::shared_ptr<RenderTarget>& target, float r, float g, float b, float a) = 0;
        virtual void SetGameRenderTarget(const std::shared_ptr<RenderTarget>& target) = 0;
        virtual void SetSwapchainClearColor(float r, float g, float b, float a) = 0;

        virtual void BeginTextureBatch() = 0;
        virtual void EndAndSubmitTextureBatch() = 0;
        virtual bool IsUploadBatchActive() const = 0;

        virtual const double GetGPUTimeMS() const = 0;

    private:
        virtual double GetGPUTime() = 0;
    };
} // namespace FINALITY
