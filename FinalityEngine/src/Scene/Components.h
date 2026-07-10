#pragma once
#include <string>

#include <glm/glm.hpp>

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
}