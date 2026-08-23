#pragma once
#include <Core/Framebuffer.h>
#include "VKCore.h"

namespace FINALITY
{
    class VKFramebuffer : public Framebuffer
    {
    public:
        VKFramebuffer(VkDevice device, VkPhysicalDevice physicalDevice, const FramebufferSpecification& spec);
        virtual ~VKFramebuffer();

        void Bind() override;
        void Unbind() override;
        void Resize(uint32_t width, uint32_t height) override;

        const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
        void* GetColorAttachmentRendererID() const override { return (void*)m_ColorImageView; }

        VkRenderPass GetVKRenderPass() const { return m_RenderPass; }
        VkFramebuffer GetVKFramebuffer() const { return m_Framebuffer; }
        VkImage GetVKColorImage() const { return m_ColorImage; }
        VkSampler GetColorSampler() const { return m_ColorSampler; }

    private:
        void Invalidate(VkPhysicalDevice physicalDevice);
        void CleanUp();
        uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        FramebufferSpecification m_Specification;

        VkImage m_ColorImage = VK_NULL_HANDLE;
        VkDeviceMemory m_ColorImageMemory = VK_NULL_HANDLE;
        VkImageView m_ColorImageView = VK_NULL_HANDLE;
        VkSampler m_ColorSampler = VK_NULL_HANDLE;

        VkImage m_DepthImage = VK_NULL_HANDLE;
        VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
    };
}
