#pragma once

#include <Finality.h>
#include <glm/gtc/matrix_transform.hpp>

namespace FINALITY
{
    struct LavaMaterialLayout
    {
        float Time;
        float Speed;
        float Scale;
        float GlowIntensity;
    };

    class LavaScript : public FINALITY::ScriptableComponent
    {
    public:
        LavaScript() = default;

    private:
        void Start() override
        {
            FI_CLIENT_INFO("LavaScript Initialized!");

            if (HasComponent<MaterialComponent>())
            {
                auto& matComp = GetComponent<MaterialComponent>();
                if (matComp.MaterialInstance)
                {
                    uint32_t baseOffset = 64;

                    matComp.MaterialInstance->RegisterUniform("time",
                        baseOffset + offsetof(LavaMaterialLayout, Time), sizeof(float));

                    matComp.MaterialInstance->RegisterUniform("speed",
                        baseOffset + offsetof(LavaMaterialLayout, Speed), sizeof(float));

                    matComp.MaterialInstance->RegisterUniform("scale",
                        baseOffset + offsetof(LavaMaterialLayout, Scale), sizeof(float));

                    matComp.MaterialInstance->RegisterUniform("glowIntensity",
                        baseOffset + offsetof(LavaMaterialLayout, GlowIntensity), sizeof(float));

                    matComp.MaterialInstance->SetUniform1f("speed", 1.2f);
                    matComp.MaterialInstance->SetUniform1f("scale", 4.0f);
                    matComp.MaterialInstance->SetUniform1f("glowIntensity", 1.5f);
                }
            }
        }

        void Update(float dt) override
        {
            m_Clock += dt;

            if (HasComponent<MaterialComponent>())
            {
                auto& matComp = GetComponent<MaterialComponent>();
                if (matComp.MaterialInstance)
                {
                    matComp.MaterialInstance->SetUniform1f("time", m_Clock);
                }
            }
        }

        void OnDestroy() override
        {
            FI_CLIENT_INFO("LavaScript destroyed.");
        }

    private:
        float m_Clock = 0.0f;
    };
}
