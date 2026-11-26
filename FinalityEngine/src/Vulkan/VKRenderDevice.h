/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKRenderDevice.h.
 */
#pragma once

#include <Core/Core.h>
#include <Core/Window.h>
#include <Core/RenderDevice.h>
#include "VKCore.h"
#include <vector>
#include <memory>
#include "VkPhysicalDevices.h"
#include "VkQueue.h"
#include "VKSwapChain.h"
#include "VKUniformBuffer.h"
#include "VKTextureUploads.h"
#include "VKViewportPass.h"

#include <Renderer/Material.h>
#include "VKDeletionQueue.h"
#include "VKDescriptorAllocator.h"
#include <Core/Framebuffer.h>

#include <Core/RenderTypes.h>
#include "VKFramebuffer.h"

#include <Core/RenderTarget.h>

namespace FINALITY
{
    class VKImGUIRenderer;

    /** @todo TODO: Document FrameInstanceBuffer struct semantics, ownership and usage. */
    struct FrameInstanceBuffer
    {
        VkBuffer Buffer = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;
        uint8_t* MappedData = nullptr;
        VkDeviceSize Capacity = 0;
        VkDeviceSize CurrentOffset = 0;
    };

    /** @todo TODO: Document VKRenderDevice class semantics, ownership and usage. */
    class VKRenderDevice : public RenderDevice
    {
    private:
        void CreateInstance();
        void SetupDebugMessanger();

        void CreateSurface(const NativeWindowHandle& handle);
        void DestroySurface() const;

        void CreateDevice();
        void DestroyDevice();

        void CreateCommandBuffers(uint32_t count);
        void CreateCommandBufferPool();
        void BeginCommandBuffers(VkCommandBuffer cmdBuf, uint32_t usageFlags);

        void CreateInstanceBuffers(VkDeviceSize initialSize = 1024 * 1024 * 16);
        void DestroyInstanceBuffers();

        void RecordCommandBuffersInternal(bool TransitionToPresent, std::vector<VkCommandBuffer>& CmdBufs);

        void UploadToDynamicInstanceBuffer(
            VkBuffer& buffer,
            VkDeviceMemory& memory,
            uint8_t*& mappedData,
            VkDeviceSize& capacity,
            VkDeviceSize& currentOffset,
            const void* data,
            VkDeviceSize dataSize,
            VkBuffer& outBuffer,
            VkDeviceSize& outOffset);

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void RecreateSwapchainResources();

        void DrawQueueToTarget(VkCommandBuffer cmd, uint32_t width, uint32_t height,
                               VkDescriptorSet globalDescriptorSet,
                               VkBuffer& instanceBuffer,
                               VkDeviceMemory& instanceBufferMemory,
                               uint8_t*& instanceBufferMapped,
                               VkDeviceSize& instanceBufferCapacity,
                               VkDeviceSize& instanceBufferOffset,
                               const std::vector<RenderPacket>& queue, RenderStatistics* statistics = nullptr, const RenderView* view = nullptr);

        VkDescriptorSet GetOrCreateTargetImGuiHandle(const std::shared_ptr<RenderTarget>& target);
        void CheckTimeQuerySupport();
        double GetGPUTime() override;

    public:
        void Initialize(const NativeWindowHandle& handle) override;
        void Shutdown() override;

        void BeginFrame() override;
        void EndFrame() override;
        void PresentFrame() override;

        void WaitIdle() override
        {
            vkDeviceWaitIdle(m_Device);
        }

        void RetireResource(std::function<void()> release) override
        {
            m_FrameDeletionQueues[m_ImageIndex].Push(std::move(release));
        }

        void FlushRetiredResources() override
        {
            WaitIdle();
            for (auto& queue : m_FrameDeletionQueues)
            {
                queue.Flush();
            }
        }

        void CreateCommandBuffers(uint32_t count, VkCommandBuffer* cmdBufs) const;
        void FreeCommandBuffers(uint32_t count, VkCommandBuffer* cmdBufs);

        void Clear(float r, float g, float b, float a) override;
        void SetWindowSpec(const WindowSpec& spec) override;

        WindowSpec GetWindowSpec() override
        {
            return m_Spec;
        }

        VkDevice GetActiveDevice() const
        {
            return m_Device;
        }

        RendererAPI GetActiveApi() const override
        {
            return RendererAPI::VULKAN;
        }

        void DrawQueue(const std::vector<RenderPacket>& queue) override;
        void BeginDynamicRendering(VkCommandBuffer CmdBuf, int ImageIndex,
                                   VkClearValue* pClearColor, VkClearValue* pDepthValue);

        std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices) override;
        std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) override;
        std::shared_ptr<Mesh> CreateMesh(const std::string& meshPath) override;
        std::shared_ptr<Pipeline> CreatePipeline(const PipelineConfig& config) override;
        std::unique_ptr<Shader> CreateShader(const std::string& path) override;
        std::shared_ptr<Pipeline> CreatePostProcessPipeline(const PipelineConfig& config) override;
        void SetFrameView(const RenderView& view) override;

        void SetOverlayRenderer(VKImGUIRenderer* renderer)
        {
            m_OverlayRenderer = renderer;
        }

        std::shared_ptr<RenderTarget> CreateRenderTarget(uint32_t width, uint32_t height) override;
        void ResizeRenderTarget(const std::shared_ptr<RenderTarget>& target, uint32_t width, uint32_t height) override;
        void SetRenderTargetView(const std::shared_ptr<RenderTarget>& target, const RenderView& view) override;
        void DestroyRenderTargetGpuResources(RenderTarget& target) override;
        void RenderSceneToTarget(const std::shared_ptr<RenderTarget>& target, const std::vector<RenderPacket>& queue) override;
        void* GetRenderTargetTextureHandle(const std::shared_ptr<RenderTarget>& target) override;
        void SetGameRenderTarget(const std::shared_ptr<RenderTarget>& target) override;
        void SetSwapchainClearColor(float r, float g, float b, float a) override;

        void BeginTextureBatch();
        void EndAndSubmitTextureBatch();

        bool IsUploadBatchActive() const
        {
            return m_Uploads.IsActive();
        }

        VkCommandBuffer GetActiveUploadCommandBuffer()
        {
            return m_Uploads.CommandBuffer();
        }

        void TrackStagingResource(VkBuffer buffer, VkDeviceMemory memory)
        {
            m_Uploads.Track(buffer, memory);
        }

        void RetainUploadResource(std::shared_ptr<void> resource)
        {
            m_Uploads.Retain(std::move(resource));
        }

        VkDescriptorSetLayout GetGlobalDescriptorSetLayout() const
        {
            return m_GlobalDescriptorSetLayout;
        }

        VkDescriptorSet GetGlobalDescriptorSet(uint32_t index) const
        {
            return m_GlobalDescriptorSets[index];
        }

        VkPhysicalDevice GetActivePhysicalDevice()
        {
            return m_Devices.SelectedDevice().device;
        }

        VkCommandPool GetCommandPool()
        {
            return m_CMDBufPool;
        }

        VkQueue GetGraphicsQueue()
        {
            return m_Queue.GetQueue();
        }

        VkDescriptorSetLayout GetMaterialDescriptorSetLayout() const
        {
            return m_MaterialDescriptorSetLayout;
        }

        void SubmitResourceToGarbageCollection(std::function<void()>&& cleanupOperation)
        {
            m_FrameDeletionQueues[m_ImageIndex].Push(std::move(cleanupOperation));
        }

        VkRenderPass GetActiveRenderPass() const
        {
            return static_cast<VKFramebuffer*>(m_PostProcessingFramebuffer.get())->GetVKRenderPass();
        }

        void SetPostProcessPipeline(std::shared_ptr<Pipeline> pipeline) override
        {
            m_PostProcessPipeline = std::move(pipeline);
        }

        VkDescriptorSetLayout GetPostProcessDescriptorSetLayout()
        {
            return m_PostProcessDescriptorSetLayout;
        }

        VKSwapChain* GetSwapChain()
        {
            return m_SwapChain.get();
        }

        VkApplicationInfo GetAppInfo() const
        {
            return m_AppInfo;
        }

        VkInstance GetInstance() const
        {
            return m_Instance;
        }

        uint32_t GetQueueFamily() const
        {
            return m_QueueFamily;
        }

        VkSurfaceCapabilitiesKHR GetSurfaceCaps() const
        {
            return m_Devices.SelectedDevice().surfaceCapabilities;
        }

        uint32_t GetNumImages() const
        {
            return m_SwapChain->GetImageCount();
        }

        VkFormat GetDepthFormat() const
        {
            return VK_FORMAT_D32_SFLOAT;
        }

        VkFormat GetSwapChainFormat() const
        {
            return m_SwapChain->GetVKFormat();
        }

        VkImage GetImages(int image) const
        {
            return m_SwapChain->GetVKImage(image);
        }

        void SetRenderTargetClearColor(const std::shared_ptr<RenderTarget>& target, float r, float g, float b, float a) override;

        const double GetGPUTimeMS() const override
        {
            return m_LastGPUTime;
        };

    private:
        WindowSpec m_Spec;
        VKImGUIRenderer* m_OverlayRenderer = nullptr;
        const std::vector<const char*> m_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        bool m_EnableValidationLayers = false;

        VkInstance m_Instance = nullptr;
        VkApplicationInfo m_AppInfo;
        VkDebugUtilsMessengerEXT m_DebugMessanger;
        VkSurfaceKHR m_Surface = nullptr;

        VkPhysicalDevices m_Devices;
        uint32_t m_QueueFamily;
        VkDevice m_Device;

        std::unique_ptr<VKSwapChain> m_SwapChain;
        VKQueue m_Queue;

        std::vector<VkCommandBuffer> m_CMDBuffers;
        VkCommandPool m_CMDBufPool;
        uint32_t m_ImageIndex = 0;

        VkClearColorValue m_ClearColor;

        VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_GlobalDescriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_GlobalDescriptorSets;

        VKUniformBuffer m_GlobalUBO;

        VkDescriptorSetLayout m_MaterialDescriptorSetLayout = VK_NULL_HANDLE;
        VKDescriptorAllocator m_MaterialDescriptorAllocator;

        /** @todo TODO: Document CachedMaterialDescriptor struct semantics, ownership and usage. */
        struct CachedMaterialDescriptor
        {
            VkDescriptorSet Set = VK_NULL_HANDLE;
            std::weak_ptr<Texture> TextureReference;
        };

        std::unordered_map<const Texture*, CachedMaterialDescriptor> m_MaterialDescriptorCache;

        std::vector<VKDeletionQueue> m_FrameDeletionQueues;
        std::shared_ptr<Framebuffer> m_PostProcessingFramebuffer = nullptr;

        VkDescriptorSetLayout m_PostProcessDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_PostProcessDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_PostProcessDescriptorSet = VK_NULL_HANDLE;
        std::shared_ptr<Pipeline> m_PostProcessPipeline = nullptr;
        VkSampler m_PostProcessSampler = VK_NULL_HANDLE;

        VKTextureUploads m_Uploads;

        std::vector<FrameInstanceBuffer> m_InstanceBuffers;
        bool m_PendingSwapchainRecreate = false;
        RenderView m_FrameView;
        std::unordered_map<RenderTarget*, VkDescriptorSet> m_TargetImGuiHandles;
        std::shared_ptr<RenderTarget> m_GameRenderTarget;
        std::vector<std::vector<std::unique_ptr<VKViewportPass>>> m_ViewportPasses;
        size_t m_ViewportPassCursor = 0;

        bool m_timeQuerySupported = false;
        float m_LastGPUTime = 0.0f;
        VkQueryPool m_TimeQueryPool = VK_NULL_HANDLE;
        std::vector<bool> m_ImageHasTimestamp;

        friend class VKImGUIRenderer;
    };
} // namespace FINALITY
