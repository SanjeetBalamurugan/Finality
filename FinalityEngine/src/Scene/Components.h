#pragma once
#include <string>

#include <glm/glm.hpp>
#include <Core/Mesh.h>
#include <Core/Pipeline.h>

#include <Renderer/Camera.h>
#include <Renderer/Material.h>

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
		glm::vec3 Position{ 0.0f };
		glm::vec3 Rotation{ 0.0f };
		glm::vec3 Scale{ 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
			: Position(pos), Rotation(rot), Scale(scale) {
		};
		TransformComponent(const glm::vec3& pos, const glm::vec3& rot)
			: Position(pos), Rotation(rot) {
		};
		TransformComponent(const glm::vec3& pos)
			: Position(pos) {
		};

		glm::mat4 GetTransformMatrix() const
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), Position);

			transform = glm::rotate(transform, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			transform = glm::rotate(transform, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			transform = glm::rotate(transform, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			transform = glm::scale(transform, Scale);

			return transform;
		}
	};

	struct MaterialComponent
	{
		std::shared_ptr<Material> MaterialInstance;

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent&) = default;
		MaterialComponent(const std::shared_ptr<Material>& materialInstance)
			: MaterialInstance(materialInstance) {
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

	struct CameraComponent
	{
		std::unique_ptr<Camera> CameraInstance;
		bool IsPrimary = true;

		CameraComponent(float fov, float aspect, float nearClip, float farClip)
		{
			CameraInstance = std::make_unique<Camera>(fov, aspect, nearClip, farClip);
		}
	};
}