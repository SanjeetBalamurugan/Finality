#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace FINALITY
{
    class Camera
    {
    public:
        Camera(float fovDegrees, float aspectRatio, float nearClip, float farClip)
            : m_FOV(fovDegrees), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
        {
            UpdateProjection();
            UpdateVectors();
        }

        virtual ~Camera() = default;

        const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }

        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
        }

        glm::mat4 GetViewMatrix(const glm::vec3& position, const glm::vec3& rotation) const
        {
            float yaw = glm::radians(rotation.y);
            float pitch = glm::radians(rotation.x);

            glm::vec3 forward;
            forward.x = sin(yaw) * cos(pitch);
            forward.y = sin(pitch);
            forward.z = -cos(yaw) * cos(pitch);
            forward = glm::normalize(forward);

            constexpr glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
            glm::vec3 up = glm::normalize(glm::cross(right, forward));

            return glm::lookAt(position, position + forward, up);
        }

        void SetPosition(const glm::vec3& position)
        {
            m_Position = position;
        }

        void SetRotation(const glm::vec3& rotation)
        {
            m_Rotation = rotation;
            UpdateVectors();
        }

        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetRotation() const { return m_Rotation; }

        const glm::vec3& GetForward() const { return m_Forward; }
        const glm::vec3& GetRight() const { return m_Right; }
        const glm::vec3& GetUp() const { return m_Up; }

        void SetViewportSize(float width, float height)
        {
            m_AspectRatio = width / height;
            UpdateProjection();
        }

    private:
        void UpdateProjection()
        {
            m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        }

        void UpdateVectors()
        {
            constexpr glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

            float yaw = glm::radians(m_Rotation.y);
            float pitch = glm::radians(m_Rotation.x);

            m_Forward.x = sin(yaw) * cos(pitch);
            m_Forward.y = sin(pitch);
            m_Forward.z = -cos(yaw) * cos(pitch);
            m_Forward = glm::normalize(m_Forward);

            m_Right = glm::normalize(glm::cross(m_Forward, worldUp));
            m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
        }

    private:
        float m_FOV;
        float m_AspectRatio;
        float m_NearClip;
        float m_FarClip;

        glm::mat4 m_ProjectionMatrix{ 1.0f };

        glm::vec3 m_Position{ 0.0f };
        glm::vec3 m_Rotation{ 0.0f };

        glm::vec3 m_Forward{ 0.0f, 0.0f, -1.0f };
        glm::vec3 m_Right{ 1.0f, 0.0f, 0.0f };
        glm::vec3 m_Up{ 0.0f, 1.0f, 0.0f };
    };
}