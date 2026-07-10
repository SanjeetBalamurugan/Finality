#pragma once
#include <Finality.h>

#include "MainScene.h"

class SplashScreen : public FINALITY::Scene
{
public:
    void OnInit() override {}

    void OnUpdate(float ts) override {
        FINALITY::Scene::OnUpdate(ts);
        FINALITY::RenderCommand::SetClearColor(0.0f, 0.0f, 0.2f, 1.0f);

        m_ElapsedTime += ts;
        if (m_ElapsedTime >= 3.0f) {
            auto next = std::make_unique<MainScene>();
            FINALITY::SceneManager::GetInstance().SetScene(std::move(next));
            FINALITY::SceneManager::GetInstance().ChangeScene();
        }
    }

    void OnDestroy() override {
        FINALITY::Scene::OnDestroy();
    }

private:
    float m_ElapsedTime = 0.0f;
};