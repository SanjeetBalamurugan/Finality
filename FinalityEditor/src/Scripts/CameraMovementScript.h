#pragma once

#include <Finality.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace FINALITY
{
    class CameraMovementScript : public ScriptableComponent
    {
    public:
        CameraMovementScript() = default;

    private:
        void Start() override
        {
            FI_CLIENT_INFO("CameraMovementScript Initialized!");

            if (HasComponent<TransformComponent>())
            {
                auto& transform = GetComponent<TransformComponent>();
                m_Rotation = transform.Rotation;
            }
        }

        void Update(float dt) override
        {
            if (!HasComponent<CameraComponent>() || !HasComponent<TransformComponent>())
                return;

            auto& camera = GetComponent<CameraComponent>();
            auto& transform = GetComponent<TransformComponent>();

            if (!camera.CameraInstance)
                return;

            if (Mouse::IsButtonDown(MouseButton::Right))
                RenderCommand::SetCursorMode(true);

            if (Mouse::IsButtonUp(MouseButton::Right))
                RenderCommand::SetCursorMode(false);

            if (Mouse::IsButtonHeld(MouseButton::Right))
            {
                m_Rotation.y += Mouse::GetDeltaX() * m_LookSensitivity;
                m_Rotation.x -= Mouse::GetDeltaY() * m_LookSensitivity;

                m_Rotation.x = glm::clamp(m_Rotation.x, -89.0f, 89.0f);

                if (m_Rotation.y > 360.0f)
                    m_Rotation.y -= 360.0f;
                else if (m_Rotation.y < 0.0f)
                    m_Rotation.y += 360.0f;
            }

            transform.Rotation = m_Rotation;

            constexpr glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

            float yaw = glm::radians(transform.Rotation.y);
            float pitch = glm::radians(transform.Rotation.x);

            glm::vec3 forward;
            forward.x = sin(yaw) * cos(pitch);
            forward.y = sin(pitch);
            forward.z = -cos(yaw) * cos(pitch);
            forward = glm::normalize(forward);

            glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

            float moveSpeed = m_MoveSpeed;

            if (Input::IsKeyHeld(Key::LeftShift))
                moveSpeed *= m_SprintMultiplier;

            moveSpeed *= dt;

            if (Input::IsKeyHeld(Key::W))
                transform.Position += forward * moveSpeed;

            if (Input::IsKeyHeld(Key::S))
                transform.Position -= forward * moveSpeed;

            if (Input::IsKeyHeld(Key::A))
                transform.Position -= right * moveSpeed;

            if (Input::IsKeyHeld(Key::D))
                transform.Position += right * moveSpeed;

            if (Input::IsKeyHeld(Key::E))
                transform.Position += worldUp * moveSpeed;

            if (Input::IsKeyHeld(Key::Q))
                transform.Position -= worldUp * moveSpeed;

            camera.CameraInstance->SetPosition(transform.Position);
            camera.CameraInstance->SetRotation(transform.Rotation);
        }

        void OnDestroy() override
        {
            FI_CLIENT_INFO("CameraMovementScript destroyed.");
        }

    private:
        glm::vec3 m_Rotation{ 0.0f };

        float m_MoveSpeed = 4.0f;
        float m_SprintMultiplier = 3.0f;
        float m_LookSensitivity = 0.12f;
    };
}