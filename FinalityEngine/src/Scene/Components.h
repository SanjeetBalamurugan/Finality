#pragma once
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Core/Mesh.h>
#include <Core/Pipeline.h>
#include <Renderer/Camera.h>
#include <Renderer/Material.h>

namespace FINALITY
{
	struct TagComponent
	{
		std::string tag;
		UUID uuid{};

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: tag(tag), uuid(UUID()) {}
	};

	struct TransformComponent
	{
		glm::vec3 Position{ 0.0f };
		glm::vec3 Rotation{ 0.0f };
		glm::vec3 Scale{ 1.0f };

		glm::mat4 Matrix{ 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
			: Position(pos), Rotation(rot), Scale(scale) {}
		TransformComponent(const glm::vec3& pos, const glm::vec3& rot)
			: Position(pos), Rotation(rot) {}
		TransformComponent(const glm::vec3& pos)
			: Position(pos) {}

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
		std::string MaterialIdentifier; // Path of the .fmat asset (empty for runtime-only materials)

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent& other)
			: MaterialInstance(other.MaterialInstance ? other.MaterialInstance->Clone() : nullptr),
			  MaterialIdentifier(other.MaterialIdentifier) {}
		MaterialComponent& operator=(const MaterialComponent& other)
		{
			if (this != &other)
			{
				MaterialInstance = other.MaterialInstance ? other.MaterialInstance->Clone() : nullptr;
				MaterialIdentifier = other.MaterialIdentifier;
			}
			return *this;
		}
		MaterialComponent(const std::shared_ptr<Material>& materialInstance)
			: MaterialInstance(materialInstance) {}
		MaterialComponent(const std::shared_ptr<Material>& instance, const std::string& identifier)
			: MaterialInstance(instance), MaterialIdentifier(identifier) {}
	};

	struct MeshComponent
	{
		std::shared_ptr<Mesh> MeshData;
		std::string MeshIdentifier;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(const std::shared_ptr<Mesh>& meshData)
			: MeshData(meshData) {}
		MeshComponent(const std::shared_ptr<Mesh>& meshData, const std::string& identifier)
			: MeshData(meshData), MeshIdentifier(identifier) {}
		MeshComponent(const std::string& identifier)
			: MeshData(Mesh::Create(identifier)), MeshIdentifier(identifier) {}
	};

	struct CameraComponent
	{
		std::unique_ptr<Camera> CameraInstance;
		bool IsPrimary = true;

		float FOV = 45.0f;
		float AspectRatio = 1.778f;
		float NearClip = 0.1f;
		float FarClip = 1000.0f;

		CameraComponent()
		{
			RebuildCamera();
		}

		CameraComponent(float fov, float aspect, float nearClip, float farClip)
			: FOV(fov), AspectRatio(aspect), NearClip(nearClip), FarClip(farClip)
		{
			RebuildCamera();
		}

		CameraComponent(const CameraComponent& other)
			: IsPrimary(other.IsPrimary), FOV(other.FOV), AspectRatio(other.AspectRatio), NearClip(other.NearClip), FarClip(other.FarClip)
		{
			if (other.CameraInstance)
				RebuildCamera();
		}

		CameraComponent& operator=(const CameraComponent& other)
		{
			if (this != &other)
			{
				IsPrimary = other.IsPrimary;
				FOV = other.FOV;
				AspectRatio = other.AspectRatio;
				NearClip = other.NearClip;
				FarClip = other.FarClip;
				if (other.CameraInstance)
					RebuildCamera();
				else
					CameraInstance.reset();
			}
			return *this;
		}

		CameraComponent(CameraComponent&&) noexcept = default;
		CameraComponent& operator=(CameraComponent&&) noexcept = default;

		void RebuildCamera()
		{
			CameraInstance = std::make_unique<Camera>(FOV, AspectRatio, NearClip, FarClip);
		}
	};
}