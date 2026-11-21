/**
 * @file
 * @ingroup core
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Mesh.h.
 */
#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include <Core/Core.h>
#include <string>

namespace FINALITY
{
	/** @todo TODO: Document Vertex struct semantics, ownership and usage. */
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TexCoord;
        glm::vec3 Normal{0.0f, 1.0f, 0.0f};
        glm::vec4 Tangent{1.0f, 0.0f, 0.0f, 1.0f};
	};

	/** @todo TODO: Document MeshPushConstants struct semantics, ownership and usage. */
	struct MeshPushConstants
	{
		glm::mat4 MVP;

		float CustomFloat0 = 0.0f;
		float CustomFloat1 = 0.0f;
		float Time = 0.0f;
		float Padding = 0.0f;
	};

	/** @todo TODO: Document Mesh class semantics, ownership and usage. */
	class FAPI Mesh
	{
	public:
		virtual ~Mesh() = default;
		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh() = default;

		static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices);
		static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		static std::shared_ptr<Mesh> Create(const std::string& meshPath);
        static void CalculateNormalsAndTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

		virtual void Destroy() = 0;
		virtual void UpdateVertices(const std::vector<Vertex>& vertices) = 0;

		virtual uint32_t GetVertexCount() const = 0;
		virtual uint32_t GetIndexCount() const = 0;
		virtual bool HasIndices() const = 0;

		glm::vec3 GetBoundsCenter() const { return m_BoundsCenter; }
		float GetBoundsRadius() const { return m_BoundsRadius; }
        const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
        const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
        bool Raycast(const glm::vec3& origin, const glm::vec3& direction, const glm::mat4& world, float& distance) const;

	private:
		glm::vec3 m_BoundsCenter = glm::vec3(0.0f);
		float m_BoundsRadius = 0.0f;
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
	protected:
		void ComputeBounds(const std::vector<Vertex>& vertices);
        void SetIndices(const std::vector<uint32_t>& indices) { m_Indices = indices; }
	};
}
