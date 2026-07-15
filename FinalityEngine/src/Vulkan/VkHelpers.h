#pragma once
#include "VKCore.h"

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