#pragma once

#include <Finality.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace FINALITY
{
    class TestScript : public FINALITY::ScriptableComponent
    {
    public:
        TestScript() = default;

    private:
        void Start() override
        {
            FI_CLIENT_INFO("TestScript Started!");
        }

        void Update(float dt) override
        {
            m_HelloTimer += dt;
            m_FPSTimer += dt;

            if (m_HelloTimer >= 5.0f)
            {
                FI_CLIENT_INFO("Hello");
                m_HelloTimer = 0.0f;
            }

            if (m_FPSTimer >= 2.0f)
            {
                float fps = 1.0f / dt;
                FI_CLIENT_INFO("FPS: {}", fps);
                m_FPSTimer = 0.0f;
            }

            const std::vector<Vertex> baseCubeVertices = {
                { { -0.3f, -0.3f, -0.3f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                { {  0.3f, -0.3f, -0.3f }, { 0.1f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                { {  0.3f,  0.3f, -0.3f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.1f } },
                { { -0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
                { { -0.3f, -0.3f,  0.3f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
                { {  0.3f, -0.3f,  0.3f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
                { {  0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.1f } },
                { { -0.3f,  0.3f,  0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }
            };

            m_RotationAngle += 1.0f * dt;

            float width = 1000.0f;
            float height = 600.0f;
            float aspectRatio = width / height;

            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.01f, 100.0f);
            glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.5f));

            glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), m_RotationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), m_RotationAngle * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 model = rotationX * rotationY;

            glm::mat4 mvp = projection * view * model;

            std::vector<Vertex> animatedVertices = baseCubeVertices;

            for (auto& vertex : animatedVertices)
            {
                glm::vec4 projectedPos = mvp * glm::vec4(vertex.Position, 1.0f);
                if (projectedPos.w != 0.0f)
                {
                    vertex.Position = glm::vec3(
                        projectedPos.x / projectedPos.w,
                        projectedPos.y / projectedPos.w,
                        projectedPos.z / projectedPos.w
                    );
                }
            }

            if (HasComponent<MeshComponent>())
            {
                auto& meshComp = GetComponent<MeshComponent>();
                meshComp.MeshData->UpdateVertices(animatedVertices);
            }
        }

        void OnDestroy() override
        {
            FI_CLIENT_INFO("Test Script is destroyed");
        }

    private:
        float m_HelloTimer = 0.0f;
        float m_FPSTimer = 0.0f;
        float m_RotationAngle = 0.0f;
    };
}
