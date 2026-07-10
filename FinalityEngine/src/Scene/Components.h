#pragma once
#include <string>

#include <glm/glm.hpp>
#include <Core/Mesh.h>
#include <Core/Pipeline.h>

namespace FINALITY
{
	struct TagComponent
	{
		std::string tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: tag(tag) {};
	};

	struct TransformComponent
	{
		glm::vec3 Position{ 1.0f };
		glm::vec3 Rotation{ 1.0f };
		glm::vec3 Scale{ 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
			: Position(pos), Rotation(rot), Scale(scale) {};
		TransformComponent(const glm::vec3& pos, const glm::vec3& rot)
			: Position(pos), Rotation(rot) {};
		TransformComponent(const glm::vec3& pos)
			: Position(pos) {};
	};

	struct MaterialComponent
	{
		std::shared_ptr<Pipeline> PipelineInstance;

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent&) = default;
		MaterialComponent(const std::shared_ptr<Pipeline>& pipeline)
			: PipelineInstance(pipeline) {
		};
	};

	struct MeshComponent
	{
		std::shared_ptr<Mesh> MeshData;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(const std::shared_ptr<Mesh>& meshData)
			: MeshData(meshData) {
		};
	};
}