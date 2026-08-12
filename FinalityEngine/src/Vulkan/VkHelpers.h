#pragma once
#include "VKCore.h"
#include <Core/Mesh.h>
#include <Core/RenderDevice.h>

inline PFN_vkCmdBeginRenderingKHR pfnCmdBeginRenderingKHR = nullptr;
inline PFN_vkCmdEndRenderingKHR pfnCmdEndRenderingKHR = nullptr;

static VkSurfaceFormatKHR ChooseSurfaceFormatAndColorSpace(const std::vector<VkSurfaceFormatKHR>& surfaceFormats)
{
	for (size_t i = 0; i < surfaceFormats.size(); i++)
	{
		if ((surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB) &&
			(surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
		{
			return surfaceFormats[i];
		}
	}

	return surfaceFormats[0];
}

static uint32_t ChooseNumImages(const VkSurfaceCapabilitiesKHR& Capabilities)
{
	uint32_t RequestedNumImages = Capabilities.minImageCount + 1;
	int FinalNumImages = 0;

	if ((Capabilities.maxImageCount > 0) && (RequestedNumImages > Capabilities.maxImageCount))
	{
		FinalNumImages = Capabilities.maxImageCount;
	}
	else
	{
		FinalNumImages = RequestedNumImages;
	}

	return FinalNumImages;
}

static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& PresentModes)
{
	for (size_t i = 0; i < PresentModes.size(); i++)
	{
		if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) // Using Mailbox mode
		{
			FI_CORE_INFO("USING MAILBOX MODE");
			return PresentModes[i];
		}
		else if (PresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			FI_CORE_INFO("USING IMMEDIATE MODE");
			return PresentModes[i];
		}
	}

	FI_CORE_INFO("USING FIFO MODE");
	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
	VkImageViewType viewType, uint32_t layerCount, uint32_t mipLevels)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.flags = 0;
	viewInfo.image = image;
	viewInfo.format = format;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.components = {
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY
	};
	viewInfo.subresourceRange = {
		.aspectMask = aspectFlags,
		.baseMipLevel = 0,
		.levelCount = mipLevels,
		.baseArrayLayer = 0,
		.layerCount = layerCount,
	};

	VkImageView imageView;
	VkResult res = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
	CHECK_VK_RESULT(res, "vkCreateImageView");

	return imageView;
}

static VkSemaphore VkCreateSemaphore(VkDevice device)
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	VkSemaphore semaphore;
	VkResult res = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);
	CHECK_VK_RESULT(res, "vkCreateSemaphore");

	return semaphore;
}

inline VkPipelineVertexInputStateCreateInfo CreatePipelineVertexInputState(
    std::vector<VkVertexInputBindingDescription>& outBindings,
    std::vector<VkVertexInputAttributeDescription>& outAttributes)
{
    outBindings.clear();
    outAttributes.clear();

    VkVertexInputBindingDescription meshBinding{};
    meshBinding.binding = 0;
    meshBinding.stride = sizeof(FINALITY::Vertex);
    meshBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Advances per vertex
    outBindings.push_back(meshBinding);

    // Location 0: Position
    outAttributes.push_back(VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(FINALITY::Vertex, Position)
        });

    // Location 1: Color
	outAttributes.push_back(VkVertexInputAttributeDescription{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(FINALITY::Vertex, Color)
		});

    // Location 2: TexCoord
    outAttributes.push_back(VkVertexInputAttributeDescription{
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(FINALITY::Vertex, TexCoord)
        });

    VkVertexInputBindingDescription instanceBinding{};
    instanceBinding.binding = 1;
    instanceBinding.stride = sizeof(FINALITY::InstancePayload); // 128 bytes
    instanceBinding.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE; // Advances per instance
    outBindings.push_back(instanceBinding);

    // Locations 3, 4, 5, 6: Transform Matrix (glm::mat4 consumes 4 consecutive vec4 locations)
    for (uint32_t i = 0; i < 4; ++i)
    {
        outAttributes.push_back(VkVertexInputAttributeDescription{
            .location = 3 + i, // Locations 3, 4, 5, 6
            .binding = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT, // 16-byte vec4 row
            .offset = static_cast<uint32_t>(offsetof(FINALITY::InstancePayload, Transform) + (i * sizeof(glm::vec4)))
            });
    }

    // Locations 7, 8, 9, 10: Custom Data (64 bytes mapped to 4 x vec4 locations)
    for (uint32_t i = 0; i < 4; ++i)
    {
        outAttributes.push_back(VkVertexInputAttributeDescription{
            .location = 7 + i, // Locations 7, 8, 9, 10
            .binding = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(FINALITY::InstancePayload, CustomData) + (i * sizeof(glm::vec4)))
            });
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(outBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = outBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(outAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = outAttributes.data();

    return vertexInputInfo;
}

inline bool HasStencilComponent(VkFormat Format)
{
	return ((Format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
		(Format == VK_FORMAT_D24_UNORM_S8_UINT));
}

// Copied from the "3D Graphics Rendering Cookbook"
static void ImageMemBarrier(VkCommandBuffer CmdBuf, VkImage Image, VkFormat Format,
	VkImageLayout OldLayout, VkImageLayout NewLayout, int LayerCount, uint32_t MipLevels, uint32_t BaseMipLevel)
{
	if (LayerCount <= 0) {
		printf("ImageMemBarrier called with LayerCount=%d!\n", LayerCount);
		exit(1);
	}

	VkImageMemoryBarrier Barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = OldLayout,
		.newLayout = NewLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = Image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = BaseMipLevel,
			.levelCount = MipLevels,
			.baseArrayLayer = 0,
			.layerCount = (uint32_t)LayerCount
		}
	};

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;

	if (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(Format == VK_FORMAT_D16_UNORM) ||
		(Format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(Format == VK_FORMAT_D32_SFLOAT) ||
		(Format == VK_FORMAT_S8_UINT) ||
		(Format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(Format == VK_FORMAT_D24_UNORM_S8_UINT))
	{
		Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent(Format)) {
			Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_GENERAL) {
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} /* Convert back from read-only to updateable */
	else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} /* Convert from updateable texture to shader read-only */
	else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} /* Convert depth texture from undefined state to depth-stencil buffer */
	else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} /* Wait for render pass to complete */
	else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		Barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
		Barrier.dstAccessMask = 0;
		/*
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		///		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		*/
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} /* Convert back from read-only to color attachment */
	else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	} /* Convert from updateable texture to shader read-only */
	else if (OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} /* Convert back from read-only to depth attachment */
	else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	} /* Convert from updateable depth texture to shader read-only */
	else if (OldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		Barrier.dstAccessMask = 0;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (OldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		Barrier.srcAccessMask = 0;
		Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else {
		printf("Unknown Barrier case: old=%d new=%d\n", OldLayout, NewLayout);
		exit(1);
	}

	vkCmdPipelineBarrier(CmdBuf, sourceStage, destinationStage,
		0, 0, NULL, 0, NULL, 1, &Barrier);
}