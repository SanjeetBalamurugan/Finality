/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKTextureUploads.h.
 */
#pragma once
#include "VKCore.h"
#include <vector>
#include <memory>

namespace FINALITY
{
	/** @todo TODO: Document TextureUploadBatchContext struct semantics, ownership and usage. */
	struct TextureUploadBatchContext {
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		std::vector<VkBuffer> stagingBuffers;
		std::vector<VkDeviceMemory> stagingMemories;
		bool isActive = false;
	};

    /** @todo TODO: Document VKTextureUploads class semantics, ownership and usage. */
    class VKTextureUploads
    {
    public:
        VKTextureUploads() = default;
        VKTextureUploads(const VKTextureUploads&) = delete;
        VKTextureUploads& operator=(const VKTextureUploads&) = delete;
        void Begin(VkDevice device, VkCommandPool pool, VkQueue queue);
        void Finish();
        bool IsActive() const { return m_State.isActive; }
        VkCommandBuffer CommandBuffer() const { return m_State.commandBuffer; }
        void Retain(std::shared_ptr<void> resource) { m_Resources.push_back(std::move(resource)); }
        void Track(VkBuffer buffer, VkDeviceMemory memory)
        { m_State.stagingBuffers.push_back(buffer); m_State.stagingMemories.push_back(memory); }
    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkCommandPool m_Pool = VK_NULL_HANDLE;
        VkQueue m_Queue = VK_NULL_HANDLE;
        TextureUploadBatchContext m_State;
        std::vector<std::shared_ptr<void>> m_Resources;
    };
}
