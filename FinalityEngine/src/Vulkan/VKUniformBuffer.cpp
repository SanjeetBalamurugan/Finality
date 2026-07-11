#include "VKUniformBuffer.h"
#include <stdexcept>
#include <cstring>

namespace FINALITY
{
    void VKUniformBuffer::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, uint32_t count)
    {
        m_Device = device;
        m_BufferSize = bufferSize;

        m_Buffers.resize(count);
        m_BuffersMemory.resize(count);
        m_BuffersMapped.resize(count);

        for (uint32_t i = 0; i < count; i++)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = m_BufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create uniform buffer!");
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(m_Device, m_Buffers[i], &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_BuffersMemory[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate uniform buffer memory!");
            }

            vkBindBufferMemory(m_Device, m_Buffers[i], m_BuffersMemory[i], 0);

            vkMapMemory(m_Device, m_BuffersMemory[i], 0, m_BufferSize, 0, &m_BuffersMapped[i]);
        }
    }

    void VKUniformBuffer::Update(uint32_t imageIndex, const void* data)
    {
        std::memcpy(m_BuffersMapped[imageIndex], data, m_BufferSize);
    }

    void VKUniformBuffer::Shutdown()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            for (size_t i = 0; i < m_Buffers.size(); i++)
            {
                vkUnmapMemory(m_Device, m_BuffersMemory[i]);
                vkDestroyBuffer(m_Device, m_Buffers[i], nullptr);
                vkFreeMemory(m_Device, m_BuffersMemory[i], nullptr);
            }
            m_Device = VK_NULL_HANDLE;
        }
    }

    uint32_t VKUniformBuffer::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable uniform memory type!");
    }
}
