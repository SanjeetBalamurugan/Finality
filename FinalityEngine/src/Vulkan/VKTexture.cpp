#include "VKTexture.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>

#ifndef GLM_CONFIG_HIJACK
#define GLM_CONFIG_HIJACK
#define GLM_FORCE_PURE
#define GLM_FORCE_UNRESTRICTED_FLOAT
#define GLM_STATIC_ASSERT(x, message) static_assert(x, message) 
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <gli/gli.hpp>
#include <filesystem>

namespace FINALITY
{
    VKTexture::VKTexture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandBuffer sharedCmd,
        const std::string& filePath, VkBuffer& outStagingBuffer, VkDeviceMemory& outStagingMemory)
        : m_Device(device)
    {
        CreateTextureImage(physicalDevice, sharedCmd, filePath, outStagingBuffer, outStagingMemory);
        CreateTextureImageView();
        CreateTextureSampler();
    }

    VKTexture::~VKTexture()
    {
        if (m_Sampler) vkDestroySampler(m_Device, m_Sampler, nullptr);
        if (m_ImageView) vkDestroyImageView(m_Device, m_ImageView, nullptr);
        if (m_TextureImage) vkDestroyImage(m_Device, m_TextureImage, nullptr);
        if (m_TextureImageMemory) vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);
    }

    void VKTexture::Bind(uint32_t slot) const {}

    void VKTexture::CreateTextureImage(VkPhysicalDevice physicalDevice, VkCommandBuffer sharedCmd,
        const std::string& filePath, VkBuffer& outStagingBuffer, VkDeviceMemory& outStagingMemory)
    {
        VkDeviceSize imageSize = 0;
        const void* pixelsToCopy = nullptr;
        bool isCompressed = false;
        gli::texture gliTex;

        std::filesystem::path path(filePath);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

        if (ext == ".dds" || ext == ".ktx" || ext == ".ktx2")
        {
            isCompressed = true;
            gliTex = gli::load(filePath);
            if (gliTex.empty()) throw std::runtime_error("Failed to load compressed texture via GLI: " + filePath);

            m_Width = static_cast<uint32_t>(gliTex.extent().x);
            m_Height = static_cast<uint32_t>(gliTex.extent().y);
            m_MipLevels = static_cast<uint32_t>(gliTex.levels());
            imageSize = gliTex.size();
            pixelsToCopy = gliTex.data();
            m_Format = static_cast<VkFormat>(gliTex.format());
        }
        else
        {
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!pixels) throw std::runtime_error("Failed to decode uncompressed texture via STB: " + filePath);

            m_Width = static_cast<uint32_t>(texWidth);
            m_Height = static_cast<uint32_t>(texHeight);
            m_MipLevels = 1;
            imageSize = m_Width * m_Height * 4;
            pixelsToCopy = pixels;
            m_Format = VK_FORMAT_R8G8B8A8_UNORM;
        }

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &outStagingBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create staging buffer!");

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(m_Device, outStagingBuffer, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &outStagingMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate staging buffer memory!");

        vkBindBufferMemory(m_Device, outStagingBuffer, outStagingMemory, 0);

        void* data;
        vkMapMemory(m_Device, outStagingMemory, 0, imageSize, 0, &data);
        std::memcpy(data, pixelsToCopy, imageSize);
        vkUnmapMemory(m_Device, outStagingMemory);

        if (!isCompressed) stbi_image_free(const_cast<void*>(pixelsToCopy));

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_Width;
        imageInfo.extent.height = m_Height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = m_MipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = m_Format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(m_Device, &imageInfo, nullptr, &m_TextureImage) != VK_SUCCESS)
            throw std::runtime_error("Failed to create hardware texture image allocation!");

        vkGetImageMemoryRequirements(m_Device, m_TextureImage, &memReqs);
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_TextureImageMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate texture image memory!");

        vkBindImageMemory(m_Device, m_TextureImage, m_TextureImageMemory, 0);

        TransitionImageLayout(sharedCmd, m_TextureImage, m_Format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        std::vector<VkBufferImageCopy> bufferCopyRegions;

        if (isCompressed)
        {
            VkDeviceSize bufferOffset = 0;
            for (uint32_t level = 0; level < m_MipLevels; ++level)
            {
                gli::texture::extent_type extent = gliTex.extent(level);

                VkBufferImageCopy region = {};
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = level;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;
                region.imageExtent.width = static_cast<uint32_t>(extent.x);
                region.imageExtent.height = static_cast<uint32_t>(extent.y);
                region.imageExtent.depth = 1;
                region.bufferOffset = bufferOffset;

                bufferCopyRegions.push_back(region);
                bufferOffset += gliTex.size(level);
            }
        }
        else
        {
            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = m_Width;
            region.imageExtent.height = m_Height;
            region.imageExtent.depth = 1;
            region.bufferOffset = 0;

            bufferCopyRegions.push_back(region);
        }

        vkCmdCopyBufferToImage(sharedCmd, outStagingBuffer, m_TextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

        TransitionImageLayout(sharedCmd, m_TextureImage, m_Format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VKTexture::CreateTextureImageView()
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_TextureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_Format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = m_MipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture image view!");
        }
    }

    void VKTexture::CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(m_MipLevels);

        if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    uint32_t VKTexture::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
                return i;
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void VKTexture::TransitionImageLayout(VkCommandBuffer sharedCmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = m_MipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else 
        {
            throw std::invalid_argument("Unsupported pipeline layout transition configuration!"); 
        }

        vkCmdPipelineBarrier(sharedCmd,
            sourceStage, destinationStage,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
}