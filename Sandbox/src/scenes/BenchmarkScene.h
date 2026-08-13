#pragma once

#include <Finality.h>
#include <Assets/AssetManager.h>
#include <chrono>
#include <vector>
#include <memory>
#include <scripts/CameraMovementScript.h>
#include <scripts/TestScript.h>

#include "MainScene.h"

class BenchmarkScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        FINALITY::Scene::OnInit();

        auto cameraEntity = CreateEntity("Benchmark Camera");
        auto& camComp = cameraEntity.AddComponent<FINALITY::CameraComponent>(
            45.0f,
            1280.0f / 720.0f,
            0.01f,
            500.0f
        );

        auto& cameraTransform =
            cameraEntity.GetComponent<FINALITY::TransformComponent>();

        cameraTransform.Position = glm::vec3(0.0f, 5.0f, 15.0f);
        cameraTransform.Rotation = glm::vec3(-10.0f, 0.0f, 0.0f);

        FINALITY::Renderer::SetActiveCamera(camComp.CameraInstance.get());

        cameraEntity.AddScript<FINALITY::CameraMovementScript>();

        std::vector<FINALITY::Vertex> cubeVertices = {
            {{-0.3f, -0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 0.3f, -0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 0.3f,  0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
            {{-0.3f,  0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.3f, -0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.3f, -0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.3f,  0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.3f,  0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
            {{-0.3f,  0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 0.3f,  0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 0.3f,  0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
            {{-0.3f,  0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
            {{-0.3f, -0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 0.3f, -0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 0.3f, -0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
            {{-0.3f, -0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.3f, -0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 0.3f, -0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 0.3f,  0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.3f,  0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
            {{-0.3f, -0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.3f, -0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.3f,  0.3f,  0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
            {{-0.3f,  0.3f, -0.3f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}
        };

        std::vector<uint32_t> cubeIndices = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };

        std::shared_ptr<FINALITY::Mesh> cubeMesh =
            FINALITY::Mesh::Create(cubeVertices, cubeIndices);

        FINALITY::RenderDevice* device =
            FINALITY::Renderer::GetDevice();

        auto vertexShader =
            FINALITY::AssetManager::CreateAsset<FINALITY::Shader>(
                "assets/shaders/shader.vert",
                device
            );

        auto fragmentShader =
            FINALITY::AssetManager::CreateAsset<FINALITY::Shader>(
                "assets/shaders/shader.frag",
                device
            );

        FINALITY::PipelineConfig cubeConfig{};
        cubeConfig.VertexShader = std::move(vertexShader);
        cubeConfig.FragmentShader = std::move(fragmentShader);
        cubeConfig.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> materialPipeline =
            FINALITY::Pipeline::Create(cubeConfig);

        auto sharedMaterial =
            std::make_shared<FINALITY::Material>(materialPipeline);

        auto pngTexture =
            FINALITY::AssetManager::CreateAsset<FINALITY::Texture>(
                "assets/textures/test.png"
            );

        sharedMaterial->SetTexture("textureSampler", pngTexture);

        constexpr int GRID_X = 20;
        constexpr int GRID_Y = 10;
        constexpr int GRID_Z = 20;
        constexpr float SPACING = 1.5f;

        m_TotalEntities = 0;
        m_FrameCount = 0;
        m_AccumulatedTime = 0.0f;
        m_UiFps = 0.0f;
        m_UiAvgFrameTimeMs = 0.0f;
        m_HistoryOffset = 0;
        m_ShowPerformanceOverlay = false;
        m_F3PressedLastFrame = false;

        for (float& frameTime : m_FrameTimeHistory)
            frameTime = 0.0f;

        for (int x = 0; x < GRID_X; x++)
        {
            for (int y = 0; y < GRID_Y; y++)
            {
                for (int z = 0; z < GRID_Z; z++)
                {
                    auto entity = CreateEntity(
                        "Cube_" +
                        std::to_string(x) + "_" +
                        std::to_string(y) + "_" +
                        std::to_string(z)
                    );

                    auto& transform =
                        entity.GetComponent<FINALITY::TransformComponent>();

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

        FINALITY::RenderCommand::SetClearColor(
            0.05f,
            0.02f,
            0.02f,
            1.0f
        );

        if (FINALITY::Input::IsKeyUp(FINALITY::Key::L))
        {
            std::cout << "L key pressed, changing scene!" << std::endl;
            FINALITY::SceneManager::GetInstance().SetScene(std::move(next));
            FINALITY::SceneManager::GetInstance().ChangeScene();
        }

        const bool f3Down =
            FINALITY::Input::IsKeyDown(FINALITY::Key::F3);

        if (f3Down && !m_F3PressedLastFrame)
            m_ShowPerformanceOverlay = !m_ShowPerformanceOverlay;

        m_F3PressedLastFrame = f3Down;

        m_FrameCount++;
        m_AccumulatedTime += ts;

        m_FrameTimeHistory[m_HistoryOffset] =
            ts * 1000.0f;

        m_HistoryOffset =
            (m_HistoryOffset + 1) % 120;

        const auto now =
            std::chrono::steady_clock::now();

        const float elapsed =
            std::chrono::duration<float>(
                now - m_LastLogTime
            ).count();

        if (elapsed >= 1.0f)
        {
            if (m_FrameCount > 0)
            {
                m_UiFps =
                    static_cast<float>(m_FrameCount) /
                    elapsed;

                m_UiAvgFrameTimeMs =
                    (m_AccumulatedTime /
                        static_cast<float>(m_FrameCount)) *
                    1000.0f;
            }

            m_FrameCount = 0;
            m_AccumulatedTime = 0.0f;
            m_LastLogTime = now;
        }

        if (!m_ShowPerformanceOverlay)
            return;

        FINALITY::PanelFlags overlayFlags =
            FINALITY::PanelFlags_NoDecoration |
            FINALITY::PanelFlags_AlwaysAutoResize |
            FINALITY::PanelFlags_NoSavedSettings |
            FINALITY::PanelFlags_NoFocusOnAppearing |
            FINALITY::PanelFlags_NoNav |
            FINALITY::PanelFlags_NoMove;

        FINALITY::ImGUIPanel::SetNextWindowPos(
            glm::vec2(10.0f, 10.0f),
            FINALITY::PanelCondition_Always,
            glm::vec2(0.0f, 0.0f)
        );

        FINALITY::ImGUIPanel::SetNextWindowBgAlpha(0.80f);

        FINALITY::ImGUIPanel perfPanel(
            "Performance Overlay",
            nullptr,
            overlayFlags
        );

        perfPanel.Text("System Benchmarks");
        perfPanel.Text("FPS: %.1f", m_UiFps);

        const int latestIndex =
            (m_HistoryOffset + 119) % 120;

        perfPanel.Text(
            "Frame Time: %.2f ms",
            m_FrameTimeHistory[latestIndex]
        );

        perfPanel.Text(
            "Avg (1s): %.2f ms",
            m_UiAvgFrameTimeMs
        );

        perfPanel.PlotLines(
            "##TimeGraph",
            m_FrameTimeHistory,
            120,
            m_HistoryOffset,
            "Frame History (ms)",
            0.0f,
            33.3f,
            glm::vec2(200.0f, 60.0f)
        );

        perfPanel.Text("Renderer Statistics");

        const uint32_t currentVisible =
            FINALITY::Renderer::GetSubmittedCount();

        const uint32_t currentCulled =
            FINALITY::Renderer::GetCulledCount();

        const float cullPercent =
            m_TotalEntities > 0
            ? 100.0f *
            static_cast<float>(currentCulled) /
            static_cast<float>(m_TotalEntities)
            : 0.0f;

        perfPanel.Text(
            "Total Entities: %u",
            m_TotalEntities
        );

        perfPanel.Text(
            "Visible Count: %u",
            currentVisible
        );

        perfPanel.Text(
            "Culled Count: %u (%.1f%%)",
            currentCulled,
            cullPercent
        );
    }

    void OnDestroy() override
    {
        FINALITY::Scene::OnDestroy();
    }

private:
    std::unique_ptr<MainScene> next = std::make_unique<MainScene>();
    uint32_t m_TotalEntities = 0;
    uint32_t m_FrameCount = 0;

    float m_AccumulatedTime = 0.0f;
    float m_UiFps = 0.0f;
    float m_UiAvgFrameTimeMs = 0.0f;

    std::chrono::steady_clock::time_point m_LastLogTime;

    bool m_ShowPerformanceOverlay = false;
    bool m_F3PressedLastFrame = false;

    float m_FrameTimeHistory[120]{};
    int m_HistoryOffset = 0;
};