#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include <Core/Core.h>

namespace FINALITY
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TexCoord;
	};

	struct MeshPushConstants
	{
		glm::mat4 MVP;

		float CustomFloat0 = 0.0f;
		float CustomFloat1 = 0.0f;
		float Time = 0.0f;
		float Padding = 0.0f;
	};

	class FAPI Mesh
	{
	public:
		virtual ~Mesh() = default;
		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh() = default;

		static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices);
		static std::shared_ptr<Mesh> Create(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

		virtual void Destroy() = 0;
		virtual void UpdateVertices(const std::vector<Vertex>& vertices) = 0;

		virtual uint32_t GetVertexCount() const = 0;
		virtual uint32_t GetIndexCount() const = 0;
		virtual bool HasIndices() const = 0;
	};
}
