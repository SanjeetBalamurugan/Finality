#pragma once
#include <Finality.h>
#include "scripts/TestScript.h"
#include <Core/Mesh.h>
#include "Renderer/Renderer.h"

class MainScene : public FINALITY::Scene
{
public:
    void OnInit() override
    {
        auto entity = CreateEntity("Rotating Cube");
        entity.AddScript<FINALITY::TestScript>();

        // 8 Corners of a 3D Cube with distinct face colors
        std::vector<FINALITY::Vertex> vertices = {
            { { -0.3f, -0.3f, -0.3f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
            { {  0.3f, -0.3f, -0.3f }, { 0.1f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
            { {  0.3f,  0.3f, -0.3f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
            { { -0.3f,  0.3f, -0.3f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
            { { -0.3f, -0.3f,  0.3f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
            { {  0.3f, -0.3f,  0.3f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
            { {  0.3f,  0.3f,  0.3f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
            { { -0.3f,  0.3f,  0.3f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }
        };

        // 36 Indices mapping out the two triangles per face
        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,
            1, 5, 6, 6, 2, 1,
            5, 4, 7, 7, 6, 5,
            4, 0, 3, 3, 7, 4,
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4
        };

        std::shared_ptr<FINALITY::Mesh> cubeMesh = FINALITY::Mesh::Create(vertices, indices);

        FINALITY::RenderDevice* device = FINALITY::Renderer::GetDevice();

        auto vertexShader = FINALITY::Shader::Create(device, "../FinalityEngine/assets/shaders/shader.vert");
        auto fragmentShader = FINALITY::Shader::Create(device, "../FinalityEngine/assets/shaders/shader.frag");

        FINALITY::PipelineConfig config;
        config.VertexShader = std::move(vertexShader);
        config.FragmentShader = std::move(fragmentShader);
        config.EnableBlending = false;

        std::shared_ptr<FINALITY::Pipeline> materialPipeline = FINALITY::Pipeline::Create(config);

        entity.AddComponent<FINALITY::MeshComponent>(cubeMesh);
        entity.AddComponent<FINALITY::MaterialComponent>(materialPipeline);
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
