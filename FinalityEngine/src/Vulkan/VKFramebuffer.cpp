#include "VKFramebuffer.h"
#include <stdexcept>
#include "VKRenderDevice.h"

#include <Core/Application.h>

namespace FINALITY
{
    VKFramebuffer::VKFramebuffer(VkDevice device, VkPhysicalDevice physicalDevice, const FramebufferSpecification& spec)
        : m_Device(device), m_Specification(spec)
    {
        Invalidate(physicalDevice);
    }

    VKFramebuffer::~VKFramebuffer()
    {
        CleanUp();
    }

    void VKFramebuffer::CleanUp()
    {
        if (m_Framebuffer) { vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr); m_Framebuffer = VK_NULL_HANDLE; }
        if (m_RenderPass) { vkDestroyRenderPass(m_Device, m_RenderPass, nullptr); m_RenderPass = VK_NULL_HANDLE; }

        if (m_DepthImageView) { vkDestroyImageView(m_Device, m_DepthImageView, nullptr); m_DepthImageView = VK_NULL_HANDLE; }
        if (m_DepthImage) { vkDestroyImage(m_Device, m_DepthImage, nullptr); m_DepthImage = VK_NULL_HANDLE; }
        if (m_DepthImageMemory) { vkFreeMemory(m_Device, m_DepthImageMemory, nullptr); m_DepthImageMemory = VK_NULL_HANDLE; }

        if (m_ColorSampler) { vkDestroySampler(m_Device, m_ColorSampler, nullptr); m_ColorSampler = VK_NULL_HANDLE; }
        if (m_ColorImageView) { vkDestroyImageView(m_Device, m_ColorImageView, nullptr); m_ColorImageView = VK_NULL_HANDLE; }
        if (m_ColorImage) { vkDestroyImage(m_Device, m_ColorImage, nullptr); m_ColorImage = VK_NULL_HANDLE; }
        if (m_ColorImageMemory) { vkFreeMemory(m_Device, m_ColorImageMemory, nullptr); m_ColorImageMemory = VK_NULL_HANDLE; }
    }

    uint32_t VKFramebuffer::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable framebuffer memory type!");
    }

    void FINALITY::VKFramebuffer::Invalidate(VkPhysicalDevice physicalDevice)
    {
        if (m_Framebuffer)
        {
            CleanUp();
        }

        VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

        VkImageCreateInfo colorImageInfo{};
        colorImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        colorImageInfo.imageType = VK_IMAGE_TYPE_2D;
        colorImageInfo.extent.width = m_Specification.Width;
        colorImageInfo.extent.height = m_Specification.Height;
        colorImageInfo.extent.depth = 1;
        colorImageInfo.mipLevels = 1;
        colorImageInfo.arrayLayers = 1;
        colorImageInfo.format = colorFormat;
        colorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        colorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        colorImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        colorImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(m_Device, &colorImageInfo, nullptr, &m_ColorImage) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create offscreen framebuffer color image!");
        }

        VkMemoryRequirements colorMemReqs;
        vkGetImageMemoryRequirements(m_Device, m_ColorImage, &colorMemReqs);

        VkMemoryAllocateInfo colorAllocInfo{};
        colorAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        colorAllocInfo.allocationSize = colorMemReqs.size;
        colorAllocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, colorMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_Device, &colorAllocInfo, nullptr, &m_ColorImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate framebuffer color memory!");
        }

        vkBindImageMemory(m_Device, m_ColorImage, m_ColorImageMemory, 0);

        VkImageViewCreateInfo colorViewInfo{};
        colorViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorViewInfo.image = m_ColorImage;
        colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorViewInfo.format = colorFormat;
        colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorViewInfo.subresourceRange.baseMipLevel = 0;
        colorViewInfo.subresourceRange.levelCount = 1;
        colorViewInfo.subresourceRange.baseArrayLayer = 0;
        colorViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device, &colorViewInfo, nullptr, &m_ColorImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer color image view!");
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_ColorSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer color sampler!");
        }

        VkImageCreateInfo depthImageInfo{};
        depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageInfo.extent.width = m_Specification.Width;
        depthImageInfo.extent.height = m_Specification.Height;
        depthImageInfo.extent.depth = 1;
        depthImageInfo.mipLevels = 1;
        depthImageInfo.arrayLayers = 1;
        depthImageInfo.format = depthFormat;
        depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(m_Device, &depthImageInfo, nullptr, &m_DepthImage) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer depth image!");
        }

        VkMemoryRequirements depthMemReqs;
        vkGetImageMemoryRequirements(m_Device, m_DepthImage, &depthMemReqs);

        VkMemoryAllocateInfo depthAllocInfo{};
        depthAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        depthAllocInfo.allocationSize = depthMemReqs.size;
        depthAllocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, depthMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_Device, &depthAllocInfo, nullptr, &m_DepthImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate framebuffer depth memory!");
        }

        vkBindImageMemory(m_Device, m_DepthImage, m_DepthImageMemory, 0);

        VkImageViewCreateInfo depthViewInfo{};
        depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewInfo.image = m_DepthImage;
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = depthFormat;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.baseMipLevel = 0;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.baseArrayLayer = 0;
        depthViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device, &depthViewInfo, nullptr, &m_DepthImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer depth image view!");
        }

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dependencies[2]{};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies;

        if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create offscreen framebuffer render pass!");
        }

        VkImageView fbAttachments[] = { m_ColorImageView, m_DepthImageView };
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_RenderPass;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = fbAttachments;
        fbInfo.width = m_Specification.Width;
        fbInfo.height = m_Specification.Height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &m_Framebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to construct offscreen native hardware Framebuffer container!");
        }
    }


    void VKFramebuffer::Bind()
    {
    }

    void VKFramebuffer::Unbind()
    {
    }

    void VKFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;

        auto* renderDevice = static_cast<VKRenderDevice*>(&Application::Get().GetDevice());
        Invalidate(renderDevice->GetActivePhysicalDevice());
    }

    std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        auto* renderDevice = static_cast<VKRenderDevice*>(&Application::Get().GetDevice());

        return std::make_shared<VKFramebuffer>(
            renderDevice->GetActiveDevice(),
            renderDevice->GetActivePhysicalDevice(),
            spec
        );
    }
}


