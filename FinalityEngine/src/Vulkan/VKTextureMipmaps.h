/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKTextureMipmaps.h.
 */
#pragma once
#include "VKCore.h"
#include <algorithm>

namespace FINALITY
{
    inline void GenerateTextureMipmaps(VkCommandBuffer commands, VkImage image, uint32_t width, uint32_t height, uint32_t levels)
    {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.image = image;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        int32_t mipWidth = static_cast<int32_t>(width), mipHeight = static_cast<int32_t>(height);
        for (uint32_t level = 1; level < levels; ++level)
        {
            barrier.subresourceRange.baseMipLevel = level - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            VkImageBlit blit{};
            blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            const int32_t nextWidth = std::max(1, mipWidth / 2), nextHeight = std::max(1, mipHeight / 2);
            blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
            blit.dstOffsets[1] = { nextWidth, nextHeight, 1 };
            vkCmdBlitImage(commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            mipWidth = nextWidth;
            mipHeight = nextHeight;
        }
        barrier.subresourceRange.baseMipLevel = levels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
} // namespace FINALITY
