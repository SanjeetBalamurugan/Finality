#pragma once

#include <Finality.h>
#include <iostream>

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

                FI_CLIENT_INFO("FPS: ", fps);

                m_FPSTimer = 0.0f;
            }
        }

        void OnDestroy() override
        {
            FI_CLIENT_INFO("Test Script is destroyed");
        }

    private:
        float m_HelloTimer = 0.0f;
        float m_FPSTimer = 0.0f;
    };
}