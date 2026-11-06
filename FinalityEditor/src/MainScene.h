#pragma once
#include <Finality.h>

class MainScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        FINALITY::Scene::OnInit();
        auto camera = CreateEntity("Main Camera");
        auto& component = camera.AddComponent<FINALITY::CameraComponent>(
            60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
        auto& transform = camera.GetComponent<FINALITY::TransformComponent>();
        transform.Position = glm::vec3(0.0f, 1.5f, 5.0f);
        transform.Rotation = glm::vec3(-15.0f, 0.0f, 0.0f);
        component.CameraInstance->SetPosition(transform.Position);
        component.CameraInstance->SetRotation(transform.Rotation);
        FINALITY::Renderer::SetActiveCamera(component.CameraInstance.get());
        FINALITY::RenderCommand::SetClearColor(0.18f, 0.18f, 0.18f, 1.0f);
    }

    void OnUpdate(float dt) override
    {
        FINALITY::Scene::OnUpdate(dt);
    }
};
