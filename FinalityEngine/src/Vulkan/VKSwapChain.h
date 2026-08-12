#pragma once
#include "VKCore.h"

#include <Core/Core.h>
#include <Core/SwapChain.h>

namespace FINALITY
{
    class VKSwapChain : public SwapChain
    {
    public:
        VKSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        ~VKSwapChain() override = default;

        void Initialize(const NativeWindowHandle& handle, const WindowSpec& spec) override;
        void Recreate(const WindowSpec& spec) override;
        void Shutdown() override;

        uint32_t AcquireNextImage() override { return UINT32_MAX; }
        void Present() override {}

        uint32_t GetWidth() const override { return m_Extent.width; }
        uint32_t GetHeight() const override { return m_Extent.height; }
        uint32_t GetImageCount() const override { return static_cast<uint32_t>(m_Images.size()); }
        uint32_t GetCurrentImageIndex() const override { return m_ImageIndex; }

        VkSwapchainKHR GetVKHandle() const { return m_SwapChain; }
        VkRenderPass GetVKRenderPass() const { return m_RenderPass; }
        VkFramebuffer GetVKFramebuffer(uint32_t index) const { return m_FrameBuffers[index]; }
        VkFormat GetVKFormat() const { return m_SurfaceFormat.format; }

        VkImage GetVKImage(uint32_t index) const { return m_Images[index]; }
        const VkImageView& GetImageView(int idx) const;
        const VkImageView& GetDepthView() const;

    private:
        void CreateSwapChain(const WindowSpec& spec);
        void CreateImageViews();
        void CreateSimpleRenderPass();
        void CreateFrameBuffers();
        void Cleanup();

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const WindowSpec& spec);
        void CreateDepthResources(VkPhysicalDevice physicalDevice);

    private:
        VkDevice m_LogicalDevice = nullptr;
        VkPhysicalDevice m_PhysicalDevice = nullptr;
        VkSurfaceKHR m_Surface = nullptr;

        VkSwapchainKHR m_SwapChain = nullptr;
        VkExtent2D m_Extent{};
        VkSurfaceFormatKHR m_SurfaceFormat{};
        uint32_t m_ImageIndex = 0;

        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;
        
        VkImage m_DepthImage = VK_NULL_HANDLE;
        VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;

        std::vector<VkFramebuffer> m_FrameBuffers;
        VkRenderPass m_RenderPass = nullptr;
    };
}