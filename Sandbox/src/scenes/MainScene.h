#pragma once
#include <Finality.h>
#include "scripts/TestScript.h"
#include <scripts/KeyPrintScript.h>
#include <scripts/CameraMovementScript.h>
#include "scripts/LavaScript.h" // Include our new lava script header

class MainScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        auto cameraEntity = CreateEntity("Main Camera");
        auto& camComp = cameraEntity.AddComponent<FINALITY::CameraComponent>(45.0f, 1000.0f / 600.0f, 0.01f, 100.0f);

        auto& cameraTransform = cameraEntity.GetComponent<FINALITY::TransformComponent>();
        cameraTransform.Position = glm::vec3(0.0f, 1.5f, 5.0f);
        cameraTransform.Rotation = glm::vec3(-15.0f, 0.0f, 0.0f);

        FINALITY::Renderer::SetActiveCamera(camComp.CameraInstance.get());
        cameraEntity.AddScript<FINALITY::CameraMovementScript>();

        auto entity = CreateEntity("Rotating Cube");
        auto& cubeTransform = entity.GetComponent<FINALITY::TransformComponent>();
        cubeTransform.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        cubeTransform.Rotation = glm::vec3(0.0f);
        cubeTransform.Scale = glm::vec3(1.0f);

        entity.AddScript<FINALITY::TestScript>();
        entity.AddScript<FINALITY::KeyPrintScript>();

        std::vector<FINALITY::Vertex> cubeVertices = {
            { { -0.3f, -0.3f, -0.3f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
            { {  0.3f, -0.3f, -0.3f }, { 0.1f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
            { {  0.3f,  0.3f, -0.3f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.1f } },
            { { -0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
            { { -0.3f, -0.3f,  0.3f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.3f, -0.3f,  0.3f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.1f } },
            { { -0.3f,  0.3f,  0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }
        };

        std::vector<uint32_t> cubeIndices = {
            0, 1, 2, 2, 3, 0,
            1, 5, 6, 6, 2, 1,
            5, 4, 7, 7, 6, 5,
            4, 0, 3, 3, 7, 4,
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4
        };

        std::shared_ptr<FINALITY::Mesh> cubeMesh = FINALITY::Mesh::Create(cubeVertices, cubeIndices);
        FINALITY::RenderDevice* device = FINALITY::Renderer::GetDevice();

        auto vertexShader = FINALITY::Shader::Create(device, "../FinalityEngine/assets/shaders/shader.vert");
        auto fragmentShader = FINALITY::Shader::Create(device, "../FinalityEngine/assets/shaders/shader.frag");

        FINALITY::PipelineConfig cubeConfig;
        cubeConfig.VertexShader = std::move(vertexShader);
        cubeConfig.FragmentShader = std::move(fragmentShader);
        cubeConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> materialPipeline = FINALITY::Pipeline::Create(cubeConfig);
        auto customMaterial = std::make_shared<FINALITY::Material>(materialPipeline);

        entity.AddComponent<FINALITY::MeshComponent>(cubeMesh);
        entity.AddComponent<FINALITY::MaterialComponent>(customMaterial);


        auto planeEntity = CreateEntity("Lava Ground Plane");

        auto& planeTransform = planeEntity.GetComponent<FINALITY::TransformComponent>();
        planeTransform.Position = glm::vec3(0.0f, -0.5f, 0.0f);
        planeTransform.Rotation = glm::vec3(0.0f);
        planeTransform.Scale = glm::vec3(1.0f);

        planeEntity.AddScript<FINALITY::LavaScript>();

        std::vector<FINALITY::Vertex> planeVertices = {
            { { -5.0f, 0.0f, -5.0f }, { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f } },
            { {  5.0f, 0.0f, -5.0f }, { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f } },
            { {  5.0f, 0.0f,  5.0f }, { 0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f } },
            { { -5.0f, 0.0f,  5.0f }, { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f } }
        };

        std::vector<uint32_t> planeIndices = {
            0, 1, 2, 2, 3, 0
        };

        std::shared_ptr<FINALITY::Mesh> planeMesh = FINALITY::Mesh::Create(planeVertices, planeIndices);

        auto lavaVert = FINALITY::Shader::Create(device, "../FinalityEngine/assets/shaders/lava.vert");
        auto lavaFrag = FINALITY::Shader::Create(device, "../FinalityEngine/assets/shaders/lava.frag");

        FINALITY::PipelineConfig lavaConfig;
        lavaConfig.VertexShader = std::move(lavaVert);
        lavaConfig.FragmentShader = std::move(lavaFrag);
        lavaConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> lavaPipeline = FINALITY::Pipeline::Create(lavaConfig);
        auto lavaMaterial = std::make_shared<FINALITY::Material>(lavaPipeline);

        planeEntity.AddComponent<FINALITY::MeshComponent>(planeMesh);
        planeEntity.AddComponent<FINALITY::MaterialComponent>(lavaMaterial);
    }

    void OnUpdate(float ts) override
    {
        FINALITY::Scene::OnUpdate(ts);
        FINALITY::RenderCommand::SetClearColor(0.05f, 0.02f, 0.02f, 1.0f);
    }

    void OnDestroy() override
    {
        FINALITY::Scene::OnDestroy();
    }
};
