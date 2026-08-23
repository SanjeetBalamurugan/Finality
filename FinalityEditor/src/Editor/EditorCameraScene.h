#pragma once
#include <Finality.h>
#include <Scripts/CameraMovementScript.h>

namespace FINALITY
{
	class EditorCameraScene : public Scene
	{
	public:
		void OnInit() override
		{
			auto camEntity = CreateEntity("Editor Camera");
			auto& camComp = camEntity.AddComponent<CameraComponent>(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
			camComp.IsPrimary = false; // irrelevant here since OnRender() is never called for this scene

			auto& transform = camEntity.GetComponent<TransformComponent>();
			transform.Position = glm::vec3(0.0f, 1.5f, 5.0f);
			transform.Rotation = glm::vec3(-15.0f, 0.0f, 0.0f);

			camEntity.AddScript<CameraMovementScript>();

			m_CameraEntity = camEntity;
		}

		void OnRender() override
		{
			// Never pushes into Renderer's queue or touches SetActiveCamera --
		}

		Camera* GetCamera()
		{
			return m_CameraEntity.GetComponent<CameraComponent>().CameraInstance.get();
		}

	private:
		Entity m_CameraEntity;
	};
}