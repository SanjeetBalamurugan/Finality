#include "VKTextureUploads.h"

namespace FINALITY
{
    void VKTextureUploads::Begin(VkDevice device, VkCommandPool pool, VkQueue queue)
    {
        if (m_State.isActive)
        {
            return;
        }
        m_State.isActive = true;
        m_Device = device;
        m_Pool = pool;
        m_Queue = queue;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_Pool;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(m_Device, &allocInfo, &m_State.commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(m_State.commandBuffer, &beginInfo);
    }

    void VKTextureUploads::Finish()
    {
        if (!m_State.isActive)
        {
            return;
        }

        vkEndCommandBuffer(m_State.commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_State.commandBuffer;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence;
        vkCreateFence(m_Device, &fenceInfo, nullptr, &fence);

        vkQueueSubmit(m_Queue, 1, &submitInfo, fence);
        vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX);

        for (size_t i = 0; i < m_State.stagingBuffers.size(); ++i)
        {
            vkDestroyBuffer(m_Device, m_State.stagingBuffers[i], nullptr);
            vkFreeMemory(m_Device, m_State.stagingMemories[i], nullptr);
        }

        vkDestroyFence(m_Device, fence, nullptr);
        vkFreeCommandBuffers(m_Device, m_Pool, 1, &m_State.commandBuffer);

        m_State = TextureUploadBatchContext();
        m_Resources.clear();
    }

} // namespace FINALITY
