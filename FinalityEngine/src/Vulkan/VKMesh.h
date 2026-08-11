#pragma once
#include "VKCore.h"
#include <Core/Mesh.h>

namespace FINALITY
{
    class VKMesh : public Mesh
    {
    public:
        VKMesh(VkDevice device, VkPhysicalDevice physicalDevice, const std::vector<Vertex>& vertices);
        VKMesh(VkDevice device, VkPhysicalDevice physicalDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        ~VKMesh() override;

        VKMesh(const VKMesh&) = delete;
        VKMesh& operator=(const VKMesh&) = delete;

        void Bind(VkCommandBuffer commandBuffer) const;
        void Destroy() override;

        void UpdateVertices(const std::vector<Vertex>& vertices) override;

        uint32_t GetVertexCount() const override { return m_VertexCount; }
        uint32_t GetIndexCount() const override { return m_IndexCount; }
        bool HasIndices() const override { return m_HasIndices; }

    private:
        void CreateVertexBuffer(const std::vector<Vertex>& vertices);
        void CreateIndexBuffer(const std::vector<uint32_t>& indices);

        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    private:
        VkDevice m_Device = nullptr;
        VkPhysicalDevice m_PhysicalDevice = nullptr;

        VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
        uint32_t m_VertexCount = 0;

        VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
        uint32_t m_IndexCount = 0;
        bool m_HasIndices = false;
    };
}
