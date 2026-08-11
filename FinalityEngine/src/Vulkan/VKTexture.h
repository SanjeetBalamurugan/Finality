#pragma once
#include <Core/Application.h>

#include <Core/Texture.h>
#include "VKCore.h"

#include "VKRenderDevice.h"

namespace FINALITY
{
	class VKTexture : public Texture
	{
	public:
		// Updated constructor signature matching your decoupled, batched upload design
		VKTexture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandBuffer sharedCmd,
			const std::string& filePath, VkBuffer& outStagingBuffer, VkDeviceMemory& outStagingMemory);
		virtual ~VKTexture();

		uint32_t GetWidth() const override { return m_Width; }
		uint32_t GetHeight() const override { return m_Height; }
		uint32_t GetMipLevels() const override { return m_MipLevels; }

		void Bind(uint32_t slot = 0) const override;

		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }

	private:
		void CreateTextureImage(VkPhysicalDevice physicalDevice, VkCommandBuffer sharedCmd,
			const std::string& filePath, VkBuffer& outStagingBuffer, VkDeviceMemory& outStagingMemory);
		void CreateTextureImageView();
		void CreateTextureSampler();

		uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

		void TransitionImageLayout(VkCommandBuffer sharedCmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_MipLevels = 1;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;

		VkImage m_TextureImage = VK_NULL_HANDLE;
		VkDeviceMemory m_TextureImageMemory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};
}
