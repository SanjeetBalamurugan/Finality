/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKMesh.h.
 */
#pragma once
#include "VKCore.h"
#include <Core/Mesh.h>
#include "VKGeometryBuffer.h"

namespace FINALITY
{
    /** @todo TODO: Document VKMesh class semantics, ownership and usage. */
    class VKMesh : public Mesh
    {
    public:
        VKMesh(VKRenderDevice& device, const std::vector<Vertex>& vertices);
        VKMesh(VKRenderDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        VKMesh(VKRenderDevice& device, const std::string& meshPath);
        ~VKMesh() override;

        VKMesh(const VKMesh&) = delete;
        VKMesh& operator=(const VKMesh&) = delete;

        void Bind(VkCommandBuffer commandBuffer) const;
        void Destroy() override;

        void UpdateVertices(const std::vector<Vertex>& vertices) override;

        uint32_t GetVertexCount() const override
        {
            return m_VertexCount;
        }

        uint32_t GetIndexCount() const override
        {
            return m_IndexCount;
        }

        bool HasIndices() const override
        {
            return m_HasIndices;
        }

    private:
        void CreateVertexBuffer(const std::vector<Vertex>& vertices);
        void CreateIndexBuffer(const std::vector<uint32_t>& indices);

    private:
        VKRenderDevice& m_Device;
        std::shared_ptr<VKGeometryBuffer> m_Vertices, m_Indices;

        uint32_t m_VertexCount = 0;

        uint32_t m_IndexCount = 0;
        bool m_HasIndices = false;
    };
} // namespace FINALITY
