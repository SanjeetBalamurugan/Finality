#include "EditorLayer.h"
#include "Prefab.h"
#include "ScenePlacement.h"
#include <ImGUI/ImGuizmoBridge.h>
#include <Renderer/ViewVisibility.h>
#include "Panels/ViewportStatistics.h"
#include <imgui_internal.h>

void FINALITY::EditorLayer::DrawSceneView()
{
    FINALITY::ImGUIPanel panel("Scene", &m_ShowScene);
    if (ImGui::GetCurrentWindow()->SkipItems)
    {
        m_SceneViewActive = false;
        m_SceneImageMaximum = {};
        return;
    }

    if (panel.IsHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        m_CameraCaptured = true;
        RenderCommand::SetCursorMode(true);
    }
    m_SceneViewActive = m_CameraCaptured || panel.IsFocused() || panel.IsHovered();

    ImGui::Checkbox("Stats", &m_ShowSceneStats);
    glm::vec2 panelSize = panel.GetContentRegionAvail();
    if (panelSize.x < 1.0f || panelSize.y < 1.0f)
    {
        return;
    }

    uint32_t width = static_cast<uint32_t>(panelSize.x);
    uint32_t height = static_cast<uint32_t>(panelSize.y);

    FINALITY::RenderDevice* device = &Application::Get().GetDevice();

    device->ResizeRenderTarget(m_SceneTarget, width, height);

    Camera* editorCam = m_EditorCameraScene->GetCamera();
    editorCam->SetViewportSize(static_cast<float>(width), static_cast<float>(height));

    device->UpdateRenderTargetCamera(m_SceneTarget, editorCam);
    const RenderView view{ editorCam->GetViewMatrix(), editorCam->GetProjection() };
    auto sceneQueue = BuildViewQueue(Renderer::GetRenderQueue(), view, m_SceneTarget->Statistics, &m_SceneTarget->VisibilityState);
    AppendSceneGrid(sceneQueue, *editorCam);
    device->RenderSceneToTarget(m_SceneTarget, sceneQueue);

    void* textureHandle = device->GetRenderTargetTextureHandle(m_SceneTarget);
    panel.Image(textureHandle, panelSize);
    const auto imageMinimum = ImGui::GetItemRectMin();
    const auto imageMaximum = ImGui::GetItemRectMax();
    m_SceneImageMinimum = imageMinimum;
    m_SceneImageMaximum = imageMaximum;
    if (m_ShowSceneStats)
    {
        DrawViewportStatsOverlay(*m_SceneTarget, imageMinimum, imageMaximum);
    }
    const bool select = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (m_Session.State == EditorState::Edit && ImGui::BeginDragDropTarget())
    {
        if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityAsset"))
        {
            if (payload->DataSize == sizeof(uint64_t))
            {
                PlaceSceneAsset(*static_cast<const uint64_t*>(payload->Data), SceneDropPosition(*editorCam, imageMinimum, imageMaximum, ImGui::GetMousePos()));
            }
        }
        ImGui::EndDragDropTarget();
    }
    DrawGizmo(m_Session.SelectedEntity);
    if (select && m_Session.State == EditorState::Edit && !ImGui::GetDragDropPayload() && (m_Session.SelectedEntity == entt::null || !ImGuizmoBridge::IsActive()))
    {
        PickSceneEntity(imageMinimum, imageMaximum);
    }
}

void FINALITY::EditorLayer::DrawGameView()
{
    ImGUIPanel panel("Game", &m_ShowGame);
    if (ImGui::GetCurrentWindow()->SkipItems)
    {
        return;
    }
    ImGui::SetNextItemWidth(200);
    const char* presets[] = { "Free Aspect", "16:9 Aspect", "4:3 Aspect", "HD 1280 x 720", "Full HD 1920 x 1080", "QHD 2560 x 1440" };
    ImGui::Combo("##Resolution", &m_GameViewSettings.Preset, presets, 6);
    ImGui::SameLine();
    ImGui::Checkbox("Stats", &m_GameViewSettings.Stats);
    const auto available = panel.GetContentRegionAvail();
    if (available.x < 1 || available.y < 1)
    {
        return;
    }
    const auto resolution = m_GameViewSettings.Resolution(available);
    auto& device = Application::Get().GetDevice();
    device.ResizeRenderTarget(m_GameTarget, static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y));
    auto* camera = Renderer::GetActiveCamera();
    if (!camera)
    {
        ImGui::TextDisabled("Add a primary Camera to see the Game view.");
        return;
    }
    camera->SetViewportSize(resolution.x, resolution.y);
    const RenderView view{ camera->GetViewMatrix(), camera->GetProjection(), camera->GetOcclusionCulling() };
    device.SetRenderTargetView(m_GameTarget, view);
    auto queue = BuildViewQueue(Renderer::GetRenderQueue(), view, m_GameTarget->Statistics, &m_GameTarget->VisibilityState);
    device.RenderSceneToTarget(m_GameTarget, queue);
    const float fit = std::min(available.x / resolution.x, available.y / resolution.y);
    const glm::vec2 displayed = resolution * fit;
    const auto position = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ position.x + (available.x - displayed.x) * 0.5f, position.y + (available.y - displayed.y) * 0.5f });
    panel.Image(device.GetRenderTargetTextureHandle(m_GameTarget), displayed);
    if (m_GameViewSettings.Stats)
    {
        DrawViewportStatsOverlay(*m_GameTarget, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }
}
