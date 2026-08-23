#pragma once
#include <Core/RenderTarget.h>

#include "VKUniformBuffer.h"
#include "VKCore.h"

namespace FINALITY
{
	struct FrameInstanceBuffer;

	struct VKRenderTarget : public RenderTarget
	{
		VKUniformBuffer CameraUBO;
		VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet GlobalDescriptorSet = VK_NULL_HANDLE;
		VkBuffer InstanceBuffer = VK_NULL_HANDLE;
		VkDeviceMemory InstanceBufferMemory = VK_NULL_HANDLE;
		uint8_t* InstanceBufferMapped = nullptr;
		VkDeviceSize InstanceBufferCapacity = 0;
		VkDeviceSize InstanceBufferOffset = 0;
	};
}