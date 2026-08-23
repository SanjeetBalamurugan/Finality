#pragma once
#include <Finality.h>
#include <Scripts/CameraMovementScript.h>

class MainScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        FINALITY::Scene::OnInit();
        FI_CLIENT_INFO("MainScene::OnInit called");

        // Initialize Camera
        auto cameraEntity = CreateEntity("Main Camera");
        auto& camComp = cameraEntity.AddComponent<FINALITY::CameraComponent>(45.0f, 1280.0f / 720.0f, 0.01f, 100.0f);

        auto& cameraTransform = cameraEntity.GetComponent<FINALITY::TransformComponent>();
        cameraTransform.Position = glm::vec3(0.0f, 1.5f, 5.0f);
        cameraTransform.Rotation = glm::vec3(-15.0f, 0.0f, 0.0f);

        FINALITY::Renderer::SetActiveCamera(camComp.CameraInstance.get());
        cameraEntity.AddScript<FINALITY::CameraMovementScript>();

        // Cube Vertex Data
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

        // Shaders and Pipeline
        auto vertexShader = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/shader.vert", device);
        auto fragmentShader = FINALITY::AssetManager::CreateAsset<FINALITY::Shader>("assets/shaders/shader.frag", device);

        FINALITY::PipelineConfig cubeConfig;
        cubeConfig.VertexShader = std::move(vertexShader);
        cubeConfig.FragmentShader = std::move(fragmentShader);
        cubeConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> materialPipeline = FINALITY::Pipeline::Create(cubeConfig);

        // Single Cube Entity Creation
        auto cubeEntity = CreateEntity("Textured Cube");
        auto& cubeTransform = cubeEntity.GetComponent<FINALITY::TransformComponent>();
        cubeTransform.Position = glm::vec3(0.0f, 0.0f, 0.0f);
        cubeTransform.Rotation = glm::vec3(0.0f);
        cubeTransform.Scale = glm::vec3(1.0f);

        auto material = std::make_shared<FINALITY::Material>(materialPipeline);
        auto texture = FINALITY::AssetManager::CreateAsset<FINALITY::Texture>("assets/textures/test.png");
        material->SetTexture("textureSampler", texture);

        cubeEntity.AddComponent<FINALITY::MeshComponent>(cubeMesh);
        cubeEntity.AddComponent<FINALITY::MaterialComponent>(material);
        FINALITY::RenderCommand::SetClearColor(0.3, 0.3, 0.2, 1.0);
    }

    void OnUpdate(float ts) override
    {
        Scene::OnUpdate(ts);
        
    }
};
