#include "EditorLayer.h"
#include <ImGUI/ImGuizmoBridge.h>
#include <limits>
#include <cmath>
#include "ScenePlacement.h"
#include "Prefab.h"

void FINALITY::EditorLayer::PlaceSceneAsset(uint64_t id, const glm::vec3& position)
{
    const auto* record = AssetRegistry::Find(id);
    auto* scene = SceneManager::GetInstance().GetActiveScene();
    if (!record || !scene || m_Session.State != EditorState::Edit)
    {
        return;
    }
    const auto reference = "asset://" + std::to_string(id);
    try
    {
        Entity entity;
        if (record->Type == ProjectAssetType::Model)
        {
            entity = ModelAsset::Instantiate(*scene, reference);
        }
        else if (record->Type == ProjectAssetType::Prefab)
        {
            entity = InstantiatePrefab(*scene, AssetRegistry::Resolve(reference));
        }
        if (!entity.IsValid())
        {
            return;
        }
        entity.GetComponent<TransformComponent>().Position = position;
        m_Session.SelectEntity(entity.GetHandle());
        m_Session.Checkpoint.Request();
    }
    catch (const std::exception& error)
    {
        FI_CLIENT_ERROR("Cannot place asset: {}", error.what());
    }
}

void FINALITY::EditorLayer::FrameSelected()
{
    auto* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene || !scene->GetRegistry().valid(m_Session.SelectedEntity))
    {
        return;
    }
    glm::vec3 minimum, maximum;
    const bool bounds = HierarchyBounds(*scene, m_Session.SelectedEntity, minimum, maximum);
    const auto center = bounds ? (minimum + maximum) * 0.5f : glm::vec3(scene->GetWorldTransform(m_Session.SelectedEntity)[3]);
    const float radius = bounds ? std::max(0.25f, glm::length(maximum - minimum) * 0.5f) : 1;
    const auto* camera = m_EditorCameraScene->GetCamera();
    const float vertical = std::atan(1 / std::abs(camera->GetProjection()[1][1]));
    const float horizontal = std::atan(1 / std::abs(camera->GetProjection()[0][0]));
    const float distance = radius / std::sin(std::min(vertical, horizontal)) * 1.15f;
    m_EditorCameraScene->SetPosition(center - camera->GetForward() * distance);
}

void FINALITY::EditorLayer::InitializeSceneTools()
{
    const auto vertex = m_EngineAssets / "Editor/Shaders/editor_grid.vert";
    const auto fragment = m_EngineAssets / "Editor/Shaders/editor_grid.frag";
    if (!std::filesystem::is_regular_file(vertex) || !std::filesystem::is_regular_file(fragment))
    {
        FI_CLIENT_ERROR("Scene grid shader files are missing from {}", m_EngineAssets.string());
        return;
    }
    PipelineConfig config;
    config.VertexShader = Shader::Create(vertex.string(), Renderer::GetDevice());
    config.FragmentShader = Shader::Create(fragment.string(), Renderer::GetDevice());
    if (!config.VertexShader || !config.FragmentShader)
    {
        return;
    }
    config.EnableBlending = true;
    config.DepthWrite = false;
    config.Culling = CullMode::None;
    auto pipeline = Pipeline::Create(config);
    if (!pipeline)
    {
        return;
    }
    m_GridMaterial = std::make_shared<Material>(pipeline);
    m_GridMaterial->RegisterUniform("spacing", 64, sizeof(float));
    m_GridMaterial->RegisterUniform("fadeDistance", 68, sizeof(float));
    m_GridMaterial->SetUniform1f("spacing", 1);
    m_GridMaterial->SetUniform1f("fadeDistance", 180);
    const std::vector<Vertex> vertices = {
        { { -1000, 0, -1000 }, { 1, 1, 1 }, { 0, 0 } }, { { 1000, 0, -1000 }, { 1, 1, 1 }, { 1, 0 } }, { { 1000, 0, 1000 }, { 1, 1, 1 }, { 1, 1 } }, { { -1000, 0, 1000 }, { 1, 1, 1 }, { 0, 1 } }
    };
    m_GridMesh = Mesh::Create(vertices, { 0, 2, 1, 0, 3, 2 });
}

void FINALITY::EditorLayer::AppendSceneGrid(std::vector<RenderPacket>& queue, Camera& camera)
{
    if (!m_ShowGrid || !m_GridMesh || !m_GridMaterial)
    {
        return;
    }
    RenderPacket packet;
    packet.MeshData = m_GridMesh;
    packet.PipelineInstance = m_GridMaterial->GetPipeline();
    packet.MaterialInstance = m_GridMaterial;
    packet.CustomPushData = m_GridMaterial->GetRawDataBuffer();
    packet.Transform = glm::translate(glm::mat4(1), glm::vec3(std::floor(camera.GetPosition().x / 10) * 10, 0, std::floor(camera.GetPosition().z / 10) * 10));
    packet.EditorOverlay = true;
    queue.push_back(std::move(packet));
}

void FINALITY::EditorLayer::PickSceneEntity(const ImVec2& minimum, const ImVec2& maximum)
{
    auto* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene || maximum.x <= minimum.x || maximum.y <= minimum.y)
    {
        return;
    }
    const auto mouse = ImGui::GetMousePos();
    const float x = 2 * (mouse.x - minimum.x) / (maximum.x - minimum.x) - 1;
    const float y = 1 - 2 * (mouse.y - minimum.y) / (maximum.y - minimum.y);
    auto* camera = m_EditorCameraScene->GetCamera();
    const auto inverse = glm::inverse(camera->GetProjection() * camera->GetViewMatrix());
    auto nearPoint = inverse * glm::vec4(x, y, 0, 1);
    auto farPoint = inverse * glm::vec4(x, y, 1, 1);
    if (std::abs(nearPoint.w) < 1.0e-8f || std::abs(farPoint.w) < 1.0e-8f)
    {
        return;
    }
    const auto origin = glm::vec3(nearPoint) / nearPoint.w;
    const auto vector = glm::vec3(farPoint) / farPoint.w - origin;
    float nearest = glm::length(vector);
    const auto direction = vector / nearest;
    entt::entity selected = entt::null;
    auto& registry = scene->GetRegistry();
    for (auto entity : registry.view<MeshComponent, MaterialComponent>())
    {
        const auto& mesh = registry.get<MeshComponent>(entity);
        const auto& material = registry.get<MaterialComponent>(entity);
        if (!mesh.MeshData || !material.MaterialInstance || !material.MaterialInstance->GetPipeline())
        {
            continue;
        }
        if (mesh.MeshData->Raycast(origin, direction, scene->GetWorldTransform(entity), nearest))
        {
            selected = entity;
        }
    }
    m_Session.SelectedEntity = selected;
    m_Session.SelectedAsset = 0;
}
