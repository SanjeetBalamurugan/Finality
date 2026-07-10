#pragma once
#include <Finality.h>
#include "scripts/TestScript.h"

class MainScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        auto entity = CreateEntity("Test Entity");

        entity.AddScript<FINALITY::TestScript>();
    }

    void OnUpdate(float ts) override
    {
        FINALITY::Scene::OnUpdate(ts);
        FINALITY::RenderCommand::SetClearColor(0.0f, 0.5f, 0.0f, 1.0f);
    }

    void OnDestroy() override
    {
        FINALITY::Scene::OnDestroy();
    }
};