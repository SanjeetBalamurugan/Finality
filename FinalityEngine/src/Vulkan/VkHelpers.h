#pragma once
#include "VKCore.h"
#include <Core/Mesh.h>
#include <Core/RenderDevice.h>

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
		if (PresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			FI_CORE_INFO("USING IMMEDIATE MODE");
			return PresentModes[i];
		}
		//if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) // Using Mailbox mode
		//{
		//	FI_CORE_INFO("USING MAILBOX MODE");
		//	return PresentModes[i];
		//}
		//else 
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