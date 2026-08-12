#pragma once

#include <Finality.h>
#include <Assets/AssetManager.h>
#include <chrono>
#include <scripts/CameraMovementScript.h>
#include <scripts/TestScript.h>

class BenchmarkScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        FINALITY::Scene::OnInit();

        auto cameraEntity = CreateEntity("Benchmark Camera");
        auto& camComp = cameraEntity.AddComponent<FINALITY::CameraComponent>(45.0f, 1280.0f / 720.0f, 0.01f, 500.0f);

        auto& cameraTransform = cameraEntity.GetComponent<FINALITY::TransformComponent>();
        cameraTransform.Position = glm::vec3(0.0f, 5.0f, 15.0f);
        cameraTransform.Rotation = glm::vec3(-10.0f, 0.0f, 0.0f);

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

        FINALITY::PipelineConfig cubeConfig{};
        cubeConfig.VertexShader = std::move(vertexShader);
        cubeConfig.FragmentShader = std::move(fragmentShader);
        cubeConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> materialPipeline = FINALITY::Pipeline::Create(cubeConfig);
        auto sharedMaterial = std::make_shared<FINALITY::Material>(materialPipeline);
        auto pngTexture = FINALITY::AssetManager::CreateAsset<FINALITY::Texture>("assets/textures/test.png");
        sharedMaterial->SetTexture("textureSampler", pngTexture);

        constexpr int GRID_X = 20;
        constexpr int GRID_Y = 10;
        constexpr int GRID_Z = 20;
        constexpr float SPACING = 1.5f;

        m_TotalEntities = 0;

        for (int x = 0; x < GRID_X; x++)
        {
            for (int y = 0; y < GRID_Y; y++)
            {
                for (int z = 0; z < GRID_Z; z++)
                {
                    auto entity = CreateEntity("BenchCube");
                    auto& transform = entity.GetComponent<FINALITY::TransformComponent>();
                    transform.Position = glm::vec3(
                        (x - GRID_X * 0.5f) * SPACING,
                        (y - GRID_Y * 0.5f) * SPACING,
                        (z - GRID_Z * 0.5f) * SPACING
                    );
                    transform.Scale = glm::vec3(1.0f);

                    entity.AddComponent<FINALITY::MeshComponent>(cubeMesh);
                    entity.AddComponent<FINALITY::MaterialComponent>(sharedMaterial);
                    entity.AddScript<FINALITY::TestScript>();

                    m_TotalEntities++;
                }
            }
        }

        m_LastLogTime = std::chrono::steady_clock::now();
    }

    void OnUpdate(float ts) override
    {
        FINALITY::Scene::OnUpdate(ts);
        FINALITY::RenderCommand::SetClearColor(0.05f, 0.02f, 0.02f, 1.0f);

        m_FrameCount++;
        m_AccumulatedTime += ts;

        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - m_LastLogTime).count();

        if (elapsed >= 1.0f)
        {
            float avgFrameTimeMs = (m_AccumulatedTime / m_FrameCount) * 1000.0f;
            float fps = m_FrameCount / elapsed;

            uint32_t visible = FINALITY::Renderer::GetSubmittedCount();
            uint32_t culled = FINALITY::Renderer::GetCulledCount();

            printf("[BENCH] FPS: %.1f | Frame: %.2fms | Total: %u | Visible: %u | Culled: %u (%.1f%%)\n",
                fps, avgFrameTimeMs, m_TotalEntities, visible, culled,
                m_TotalEntities > 0 ? (100.0f * culled / m_TotalEntities) : 0.0f);

            m_FrameCount = 0;
            m_AccumulatedTime = 0.0f;
            m_LastLogTime = now;
        }
    }

    void OnDestroy() override
    {
        FINALITY::Scene::OnDestroy();
    }

private:
    uint32_t m_TotalEntities = 0;
    uint32_t m_FrameCount = 0;
    float m_AccumulatedTime = 0.0f;
    std::chrono::steady_clock::time_point m_LastLogTime;
};