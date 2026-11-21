/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for EditorCameraScene.h.
 */
#pragma once
#include <Finality.h>
#include <Scripts/CameraMovementScript.h>

namespace FINALITY
{
	/** @todo TODO: Document EditorCameraScene class semantics, ownership and usage. */
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

			m_Navigation = &camEntity.AddScript<CameraMovementScript>();
            camComp.CameraInstance->SetPosition(transform.Position);
            camComp.CameraInstance->SetRotation(transform.Rotation);

			m_CameraEntity = camEntity;
		}

        void OnUpdate(float dt) override
        {
            Scene::OnUpdate(dt);
            if (m_CameraEntity.IsValid())
            {
                auto& transform = m_CameraEntity.GetComponent<TransformComponent>();
                auto& camera = m_CameraEntity.GetComponent<CameraComponent>();
                camera.CameraInstance->SetPosition(transform.Position);
                camera.CameraInstance->SetRotation(transform.Rotation);
            }
        }

		void OnRender() override
		{
			// Never pushes into Renderer's queue or touches SetActiveCamera --
		}

		Camera* GetCamera()
		{
			return m_CameraEntity.GetComponent<CameraComponent>().CameraInstance.get();
		}
        void ConfigureNavigation(float speed, float sprint, float sensitivity)
        { if (m_Navigation) m_Navigation->Configure(speed,sprint,sensitivity); }
        void SetPosition(const glm::vec3& position)
        {
            m_CameraEntity.GetComponent<TransformComponent>().Position = position;
            GetCamera()->SetPosition(position);
        }

	private:
		Entity m_CameraEntity;
        CameraMovementScript* m_Navigation = nullptr;
	};
}
