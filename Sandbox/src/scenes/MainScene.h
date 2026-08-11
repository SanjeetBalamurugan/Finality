#pragma once
#include <Finality.h>
#include <Assets/AssetManager.h>
#include "scripts/TestScript.h"
#include <scripts/KeyPrintScript.h>
#include <scripts/CameraMovementScript.h>
#include "scripts/LavaScript.h"

class MainScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        FINALITY::Scene::OnInit();

        auto cameraEntity = CreateEntity("Main Camera");
        auto& camComp = cameraEntity.AddComponent<FINALITY::CameraComponent>(45.0f, 1280.0f / 720.0f, 0.01f, 100.0f);

        auto& cameraTransform = cameraEntity.GetComponent<FINALITY::TransformComponent>();
        cameraTransform.Position = glm::vec3(0.0f, 1.5f, 5.0f);
        cameraTransform.Rotation = glm::vec3(-15.0f, 0.0f, 0.0f);

        FINALITY::Renderer::SetActiveCamera(camComp.CameraInstance.get());
        cameraEntity.AddScript<FINALITY::CameraMovementScript>();

        std::vector<FINALITY::Vertex> cubeVertices = {
            { { -0.3f, -0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { {  0.3f, -0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { {  0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { { -0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.3f, -0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { { -0.3f, -0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { { -0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { { -0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { {  0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { {  0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { { -0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { { -0.3f, -0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { {  0.3f, -0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { {  0.3f, -0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { { -0.3f, -0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.3f, -0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { {  0.3f, -0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { {  0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { { -0.3f, -0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
            { { -0.3f, -0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { { -0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { { -0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }
        };

        std::vector<uint32_t> cubeIndices = {
            0,  1,  2,  2,  3,  0,
            4,  5,  6,  6,  7,  4,
            8,  9,  10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };

        std::shared_ptr<FINALITY::Mesh> cubeMesh = FINALITY::Mesh::Create(cubeVertices, cubeIndices);
        FINALITY::RenderDevice* device = FINALITY::Renderer::GetDevice();

        auto vertexShader = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/shader.vert", device);
        auto fragmentShader = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/shader.frag", device);

        FINALITY::PipelineConfig cubeConfig;
        cubeConfig.VertexShader = std::move(vertexShader);
        cubeConfig.FragmentShader = std::move(fragmentShader);
        cubeConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> materialPipeline = FINALITY::Pipeline::Create(cubeConfig);

        auto cube1Entity = CreateEntity("PNG Textured Cube");
        auto& cube1Transform = cube1Entity.GetComponent<FINALITY::TransformComponent>();
        cube1Transform.Position = glm::vec3(-1.0f, 0.5f, 0.0f);
        cube1Transform.Rotation = glm::vec3(0.0f);
        cube1Transform.Scale = glm::vec3(1.0f);

        auto pngMaterial = std::make_shared<FINALITY::Material>(materialPipeline);
        auto pngTexture = FINALITY::AssetManager::CreateAsset<FINALITY::Texture>("assets/textures/test.png");
        pngMaterial->SetTexture("textureSampler", pngTexture);

        cube1Entity.AddComponent<FINALITY::MeshComponent>(cubeMesh);
        cube1Entity.AddComponent<FINALITY::MaterialComponent>(pngMaterial);
        cube1Entity.AddScript<FINALITY::TestScript>();
        //cube1Entity.AddScript<FINALITY::KeyPrintScript>();

        auto cube3Entity = CreateEntity("PNG Textured Cube2");
        auto& cube3Transform = cube3Entity.GetComponent<FINALITY::TransformComponent>();
        cube3Transform.Position = glm::vec3(-2.0f, 0.5f, 0.0f);
        cube3Transform.Rotation = glm::vec3(0.0f);
        cube3Transform.Scale = glm::vec3(1.0f);

        cube3Entity.AddComponent<FINALITY::MeshComponent>(cubeMesh);
        cube3Entity.AddComponent<FINALITY::MaterialComponent>(pngMaterial);

        auto cube2Entity = CreateEntity("KTX Textured Cube");
        auto& cube2Transform = cube2Entity.GetComponent<FINALITY::TransformComponent>();
        cube2Transform.Position = glm::vec3(1.0f, 0.5f, 0.0f);
        cube2Transform.Rotation = glm::vec3(0.0f);
        cube2Transform.Scale = glm::vec3(1.0f);

        auto ktxMaterial = std::make_shared<FINALITY::Material>(materialPipeline);
        auto ktxTexture = FINALITY::AssetManager::CreateAsset<FINALITY::Texture>("assets/textures/metal.dds");
        ktxMaterial->SetTexture("textureSampler", ktxTexture);

        cube2Entity.AddComponent<FINALITY::MeshComponent>(cubeMesh);
        cube2Entity.AddComponent<FINALITY::MaterialComponent>(ktxMaterial);
        cube2Entity.AddScript<FINALITY::TestScript>();

        auto planeEntity = CreateEntity("Lava Ground Plane");
        auto& planeTransform = planeEntity.GetComponent<FINALITY::TransformComponent>();
        planeTransform.Position = glm::vec3(0.0f, -0.5f, 0.0f);
        planeTransform.Rotation = glm::vec3(0.0f);
        planeTransform.Scale = glm::vec3(1.0f);

        std::vector<FINALITY::Vertex> planeVertices = {
            { { -5.0f, 0.0f, -5.0f }, { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f } },
            { {  5.0f, 0.0f, -5.0f }, { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f } },
            { {  5.0f, 0.0f,  5.0f }, { 0.5f, 0.5f, 0.5f }, { 1.0f, 1.0f } },
            { { -5.0f, 0.0f,  5.0f }, { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f } }
        };

        std::vector<uint32_t> planeIndices = {
            0, 3, 2, 2, 1, 0
        };

        std::shared_ptr<FINALITY::Mesh> planeMesh = FINALITY::Mesh::Create(planeVertices, planeIndices);

        auto lavaVert = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/lava.vert", device);
        auto lavaFrag = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/lava.frag", device);

        FINALITY::PipelineConfig lavaConfig;
        lavaConfig.VertexShader = std::move(lavaVert);
        lavaConfig.FragmentShader = std::move(lavaFrag);
        lavaConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> lavaPipeline = FINALITY::Pipeline::Create(lavaConfig);
        auto lavaMaterial = std::make_shared<FINALITY::Material>(lavaPipeline);

        planeEntity.AddComponent<FINALITY::MeshComponent>(planeMesh);
        planeEntity.AddComponent<FINALITY::MaterialComponent>(lavaMaterial);
        planeEntity.AddScript<FINALITY::LavaScript>();

        auto ppVert = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/postprocess.vert", device);
        auto ppFrag = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/postprocess.frag", device);

        FINALITY::PipelineConfig ppConfig;
        ppConfig.VertexShader = std::move(ppVert);
        ppConfig.FragmentShader = std::move(ppFrag);
        ppConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> postProcessPipeline = FINALITY::Renderer::CreatePostProcessPipeline(ppConfig);
        FINALITY::Renderer::SetPostProcessPipeline(postProcessPipeline);
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
