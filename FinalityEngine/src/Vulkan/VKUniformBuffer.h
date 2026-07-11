#pragma once
#include "VKCore.h"
#include <vector>

namespace FINALITY
{
    class VKUniformBuffer
    {
    public:
        VKUniformBuffer() = default;
        ~VKUniformBuffer() { Shutdown(); }

        void Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, uint32_t count);
        void Shutdown();
        void Update(uint32_t imageIndex, const void* data);

        VkBuffer GetBuffer(uint32_t imageIndex) const { return m_Buffers[imageIndex]; }
        VkDeviceSize GetSize() const { return m_BufferSize; }

    private:
        uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDeviceSize m_BufferSize = 0;

        std::vector<VkBuffer> m_Buffers;
        std::vector<VkDeviceMemory> m_BuffersMemory;
        std::vector<void*> m_BuffersMapped;
    };
}
