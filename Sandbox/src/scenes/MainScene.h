#pragma once
#include <Finality.h>

class MainScene : public FINALITY::Scene
{
public:
    void OnInit() override {}

    void OnUpdate(float ts) override {
        FINALITY::RenderCommand::SetClearColor(0.0f, 0.5f, 0.0f, 1.0f);
    }

    void OnDestroy() override {}

};