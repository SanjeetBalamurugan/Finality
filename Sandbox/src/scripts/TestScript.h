#pragma once

#include <Finality.h>
#include <iostream>

namespace FINALITY
{
    struct MaterialCompLayout
    {
        // without includeing glm::mat4 64 bytes
        float Time;
    };

    class TestScript : public FINALITY::ScriptableComponent
    {
    public:
        TestScript() = default;

    private:
        void Start() override
        {
            FI_CLIENT_INFO("TestScript Started using Push Constants!");

            uint32_t baseOffset = 64;

            if (HasComponent<MaterialComponent>())
            {
                auto& materialComp = GetComponent<MaterialComponent>();
                if (materialComp.MaterialInstance)
                {
                    materialComp.MaterialInstance->RegisterUniform("time", baseOffset + offsetof(MaterialCompLayout, Time), sizeof(float));
                }
            }
        }

        void Update(float dt) override
        {
            m_HelloTimer += dt;
            m_FPSTimer += dt;
            m_TotalTime += dt;

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

            m_RotationAngle += 50.0f * dt;

            if (HasComponent<TransformComponent>())
            {
                auto& transform = GetComponent<TransformComponent>();
                transform.Rotation.x = m_RotationAngle;
                transform.Rotation.y = m_RotationAngle * 0.5f;
            }

            if (HasComponent<MaterialComponent>())
            {
                auto& materialComp = GetComponent<MaterialComponent>();
                if (materialComp.MaterialInstance)
                {
                    materialComp.MaterialInstance->SetUniform1f("time", m_TotalTime);
                }
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
        float m_TotalTime = 0.0f;
    };
}
