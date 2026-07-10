#include "VKMesh.h"
#include <stdexcept>
#include <cstring>

namespace FINALITY
{
    VKMesh::VKMesh(VkDevice device, VkPhysicalDevice physicalDevice, const std::vector<Vertex>& vertices)
        : m_Device(device), m_PhysicalDevice(physicalDevice), m_VertexCount(static_cast<uint32_t>(vertices.size())), m_HasIndices(false)
    {
        CreateVertexBuffer(vertices);
    }

    VKMesh::VKMesh(VkDevice device, VkPhysicalDevice physicalDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
        : m_Device(device), m_PhysicalDevice(physicalDevice), m_VertexCount(static_cast<uint32_t>(vertices.size())), m_IndexCount(static_cast<uint32_t>(indices.size())), m_HasIndices(true)
    {
        CreateVertexBuffer(vertices);
        CreateIndexBuffer(indices);
    }

    VKMesh::~VKMesh()
    {
        Destroy();
    }

    void VKMesh::Bind(VkCommandBuffer commandBuffer)
    {
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer, offsets);

        if (m_HasIndices)
        {
            vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void VKMesh::Destroy()
    {
        if (m_IndexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
            m_IndexBuffer = VK_NULL_HANDLE;
        }
        if (m_IndexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
            m_IndexBufferMemory = VK_NULL_HANDLE;
        }

        if (m_VertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
            m_VertexBuffer = VK_NULL_HANDLE;
        }
        if (m_VertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
            m_VertexBufferMemory = VK_NULL_HANDLE;
        }
    }

    void VKMesh::UpdateVertices(const std::vector<Vertex>& vertices)
    {
        m_VertexCount = static_cast<uint32_t>(vertices.size());
        VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

        void* data;
        vkMapMemory(m_Device, m_VertexBufferMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_Device, m_VertexBufferMemory);
    }

    void VKMesh::CreateVertexBuffer(const std::vector<Vertex>& vertices)
    {
        VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

        CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_VertexBuffer,
            m_VertexBufferMemory
        );

        void* data;
        vkMapMemory(m_Device, m_VertexBufferMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_Device, m_VertexBufferMemory);
    }

    void FINALITY::VKMesh::CreateIndexBuffer(const std::vector<uint32_t>& indices)
    {
        VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

        CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_IndexBuffer,
            m_IndexBufferMemory
        );

        void* data;
        vkMapMemory(m_Device, m_IndexBufferMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(m_Device, m_IndexBufferMemory);
    }

    void VKMesh::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create geometry data buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_Device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate data buffer memory space!");
        }

        vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
    }

    uint32_t VKMesh::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to discover suitable match for GPU hardware allocation properties!");
    }
}
