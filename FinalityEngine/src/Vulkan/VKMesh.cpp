#include "VKMesh.h"
#include "VKRenderDevice.h"

#include "Scene/Primitives/Cube.h"
#include <stdexcept>
#include <cstring>

namespace FINALITY
{
    VKMesh::VKMesh(VKRenderDevice& device, const std::vector<Vertex>& vertices)
        : m_Device(device), m_VertexCount(static_cast<uint32_t>(vertices.size())), m_HasIndices(false)
    {
        VKGeometryUploadScope upload(m_Device);
        CreateVertexBuffer(vertices);
        ComputeBounds(vertices);
    }

    VKMesh::VKMesh(VKRenderDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
        : m_Device(device), m_VertexCount(static_cast<uint32_t>(vertices.size())), m_IndexCount(static_cast<uint32_t>(indices.size())), m_HasIndices(true)
    {
        VKGeometryUploadScope upload(m_Device);
        CreateVertexBuffer(vertices);
        CreateIndexBuffer(indices);
        ComputeBounds(vertices);
    }

    VKMesh::VKMesh(VKRenderDevice& device, const std::string& meshPath)
        : m_Device(device), m_VertexCount(0), m_IndexCount(0), m_HasIndices(false)
    {
        VKGeometryUploadScope upload(m_Device);
        if (meshPath.empty())
        {
            FI_CORE_WARN("VKMesh constructor called with an empty mesh path. Creating an empty mesh.??");
        }

        if (meshPath == "builtin://cube")
        {
            const auto& vertices = Primitives::Cube::GetVertices();
            const auto& indices = Primitives::Cube::GetIndices();
            m_VertexCount = static_cast<uint32_t>(vertices.size());
            m_IndexCount = static_cast<uint32_t>(indices.size());
            m_HasIndices = true;
            std::vector<Vertex> prepared = vertices;
            Mesh::CalculateNormalsAndTangents(prepared, indices);
            CreateVertexBuffer(prepared);
            if (m_HasIndices)
            {
                CreateIndexBuffer(indices);
            }
            ComputeBounds(prepared);
        }
        else
        {
            FI_CORE_ERROR("VKMesh constructor called with an unrecognized mesh path: {}. Creating an empty mesh.", meshPath);
        }
    }

    VKMesh::~VKMesh()
    {
        Destroy();
    }

    void VKMesh::Bind(VkCommandBuffer commandBuffer) const
    {
        if (!m_Vertices || !m_VertexCount)
        {
            return;
        }
        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Vertices->Buffer, &offset);
        if (m_HasIndices && m_Indices)
        {
            vkCmdBindIndexBuffer(commandBuffer, m_Indices->Buffer, 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void VKMesh::Destroy()
    {
        if (!m_Vertices && !m_Indices)
        {
            return;
        }
        m_Device.RetireResource([vertices = std::move(m_Vertices), indices = std::move(m_Indices)]()
                                {
                                });
        m_VertexCount = m_IndexCount = 0;
        m_HasIndices = false;
    }

    void VKMesh::UpdateVertices(const std::vector<Vertex>& vertices)
    {
        for (auto index : GetIndices())
        {
            if (index >= vertices.size())
            {
                throw std::runtime_error("Updated mesh vertices do not cover its indices");
            }
        }
        VKGeometryUploadScope upload(m_Device);
        auto replacement = VKGeometryBuffer::Upload(m_Device, vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        if (m_Vertices)
        {
            m_Device.RetireResource([old = std::move(m_Vertices)]()
                                    {
                                    });
        }
        m_Vertices = std::move(replacement);
        m_VertexCount = static_cast<uint32_t>(vertices.size());
        ComputeBounds(vertices);
    }

    void VKMesh::CreateVertexBuffer(const std::vector<Vertex>& vertices)
    {
        m_Vertices = VKGeometryBuffer::Upload(m_Device, vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    void VKMesh::CreateIndexBuffer(const std::vector<uint32_t>& indices)
    {
        SetIndices(indices);
        m_Indices = VKGeometryBuffer::Upload(m_Device, indices.data(), sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
} // namespace FINALITY
