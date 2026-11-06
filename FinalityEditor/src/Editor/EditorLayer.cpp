
#include "EditorLayer.h"
#include "Panels/InspectorPanel.h"
#include <Util/Helpers.h>
#include <filesystem>
#include <Assets/MaterialAsset.h>
#include <ImGUI/ImGuizmoBridge.h>

void FINALITY::EditorLayer::Init()
{
    FINALITY::RenderDevice* device = &Application::Get().GetDevice();

    m_SceneTarget = device->CreateRenderTarget(1280, 720);
    m_GameTarget = device->CreateRenderTarget(1280, 720);

    m_SceneTarget->ClearColor = glm::vec4(0.0f, 0.05f, 0.15f, 1.0f);
    device->SetSwapchainClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    device->SetGameRenderTarget(m_GameTarget);

    m_EditorCameraScene = std::make_unique<EditorCameraScene>();
    m_EditorCameraScene->OnInit();

    Renderer::SetDirectRenderingEnabled(false);
    Renderer::SetFrustumCullingEnabled(false);
    SceneManager::GetInstance().SetSimulationEnabled(false);
    m_LastLogTime = std::chrono::steady_clock::now();
}

void FINALITY::EditorLayer::OnUpdate(float deltaTime)
{
    if (m_SceneViewActive)
    {
        m_EditorCameraScene->OnUpdate(deltaTime);
    }

    // For Performance Panel
    m_FrameCount++;
    m_AccumulatedTime += deltaTime;

    m_FrameTimeHistory[m_HistoryOffset] = deltaTime * 1000.0f;
    m_HistoryOffset = (m_HistoryOffset + 1) % 120;

    const auto now = std::chrono::steady_clock::now();

    const float elapsed = std::chrono::duration<float>(now - m_LastLogTime).count();

    if (elapsed >= 1.0f)
    {
        if (m_FrameCount > 0)
        {
            m_UiFps = static_cast<float>(m_FrameCount) / elapsed;
            m_UiAvgFrameTimeMs = (m_AccumulatedTime / static_cast<float>(m_FrameCount)) * 1000.0f;
        }

        m_FrameCount = 0;
        m_AccumulatedTime = 0.0f;
        m_LastLogTime = now;
    }

    m_ShaderReloadClock += deltaTime;
    if (m_ShaderReloadClock >= 1.0f)
    {
        m_ShaderReloadClock = 0.0f;
        AssetManager::ReloadMaterialShaders();
    }
}

void FINALITY::EditorLayer::OnImGuiRender()
{
    DrawToolbar();
    DrawMenuBar();
    DrawSceneView();
    DrawGameView();
    FINALITY::DrawConsole();

    FINALITY::DrawPerformancePanel(m_UiFps, m_HistoryOffset, m_FrameTimeHistory, m_UiAvgFrameTimeMs);

    Scene* activeScene = SceneManager::GetInstance().GetActiveScene();
    if (!activeScene) activeScene = m_EditorCameraScene.get();
    if (!activeScene->GetRegistry().valid(m_SelectedEntity))
        m_SelectedEntity = entt::null;
    DrawSceneHierarchyPanel(activeScene, m_SelectedEntity);
    DrawInspectorPanel(activeScene, m_SelectedEntity);
    DrawContentBrowser(m_SelectedEntity);
    if (m_State == EditorState::Edit)
        RecordHistory();
}

void FINALITY::EditorLayer::DrawToolbar()
{
    //FINALITY::ImGUIPanel panel("Toolbar", nullptr, FINALITY::PanelFlags_NoTitleBar | FINALITY::PanelFlags_NoResize | FINALITY::PanelFlags_NoMove);
    FINALITY::ImGUIPanel panel("Toolbar");

    if (panel.Button("Play") && m_State == EditorState::Edit)
    {
        OnPlay();
    }
    panel.SameLine();

    std::string_view pauseLabel = (m_State == EditorState::Paused) ? "Resume" : "Pause";
    if (panel.Button(pauseLabel))
    {
        if (m_State == EditorState::Play) OnPause();
        else if (m_State == EditorState::Paused) OnResume();
    }
    panel.SameLine();

    if (panel.Button("Stop") && m_State != EditorState::Edit)
    {
        OnStop();
    }
    panel.SameLine();
    if (panel.Button("Move")) m_GizmoOperation = 0;
    panel.SameLine();
    if (panel.Button("Rotate")) m_GizmoOperation = 1;
    panel.SameLine();
    if (panel.Button("Scale")) m_GizmoOperation = 2;
    panel.SameLine();
    if (panel.Button("Recompile Shaders")) AssetManager::ReloadMaterialShaders(true);
}

void FINALITY::EditorLayer::DrawSceneView()
{
    FINALITY::ImGUIPanel panel("Scene");

    m_SceneViewActive = panel.IsFocused() || panel.IsHovered();

    glm::vec2 panelSize = panel.GetContentRegionAvail();
    if (panelSize.x < 1.0f || panelSize.y < 1.0f) return;

    uint32_t width = static_cast<uint32_t>(panelSize.x);
    uint32_t height = static_cast<uint32_t>(panelSize.y);

    FINALITY::RenderDevice* device = &Application::Get().GetDevice();

    device->ResizeRenderTarget(m_SceneTarget, width, height);

    Camera* editorCam = m_EditorCameraScene->GetCamera();
    editorCam->SetViewportSize(static_cast<float>(width), static_cast<float>(height));

    device->UpdateRenderTargetCamera(m_SceneTarget, editorCam);
    device->RenderSceneToTarget(m_SceneTarget, Renderer::GetRenderQueue());

    void* textureHandle = device->GetRenderTargetTextureHandle(m_SceneTarget);
    panel.Image(textureHandle, panelSize);
    DrawGizmo(m_SelectedEntity);
}

void FINALITY::EditorLayer::DrawGameView()
{
    FINALITY::ImGUIPanel panel("Game");

    glm::vec2 panelSize = panel.GetContentRegionAvail();
    if (panelSize.x < 1.0f || panelSize.y < 1.0f) return;

    uint32_t width = static_cast<uint32_t>(panelSize.x);
    uint32_t height = static_cast<uint32_t>(panelSize.y);

    FINALITY::RenderDevice* device = &Application::Get().GetDevice();
    device->ResizeRenderTarget(m_GameTarget, width, height);

    Camera* runtimeCamera = Renderer::GetActiveCamera();
    if (runtimeCamera)
    {
        runtimeCamera->SetViewportSize(static_cast<float>(width), static_cast<float>(height));
        device->UpdateRenderTargetCamera(m_GameTarget, runtimeCamera);
        device->RenderSceneToTarget(m_GameTarget, Renderer::GetRenderQueue());
        void* textureHandle = device->GetRenderTargetTextureHandle(m_GameTarget);
        panel.Image(textureHandle, panelSize);
    }
}

void FINALITY::EditorLayer::DrawMenuBar()
{
    if (!ImGUIPanel::BeginMainMenuBar()) return;
    if (ImGUIPanel::BeginMenu("File"))
    {
        if (ImGUIPanel::MenuItem("New Scene") && m_State == EditorState::Edit) NewScene();
        if (ImGUIPanel::MenuItem("Save Scene") && m_State == EditorState::Edit) SaveScene();
        if (ImGUIPanel::MenuItem("Open Scene") && m_State == EditorState::Edit)
        {
            const std::string path = FileDialogs::OpenFile(
                "Finality Scene (*.fscene)\0*.fscene\0All Files (*.*)\0*.*\0");
            if (!path.empty()) OpenScene(path);
        }
        if (ImGUIPanel::MenuItem("Exit")) Application::Get().RequestClose();
        ImGUIPanel::EndMenu();
    }
    if (ImGUIPanel::BeginMenu("Edit"))
    {
        if (ImGUIPanel::MenuItem("Undo") && m_State == EditorState::Edit && m_HistoryIndex > 0)
            RestoreHistory(m_HistoryIndex - 1);
        if (ImGUIPanel::MenuItem("Redo") && m_State == EditorState::Edit && m_HistoryIndex + 1 < m_History.size())
            RestoreHistory(m_HistoryIndex + 1);
        ImGUIPanel::EndMenu();
    }
    ImGUIPanel::EndMainMenuBar();
}

void FINALITY::EditorLayer::NewScene()
{
    if (m_State != EditorState::Edit) return;
    auto scene = std::make_unique<EmptyScene>();
    auto camera = scene->CreateEntity("Main Camera");
    camera.AddComponent<CameraComponent>(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    SceneManager::GetInstance().SetScene(std::move(scene));
    SceneManager::GetInstance().ChangeScene();
    m_SelectedEntity = entt::null;
    m_ScenePath.clear();
    m_History.clear();
    RecordHistory();
}

bool FINALITY::EditorLayer::OpenScene(const std::string& path)
{
    if (m_State != EditorState::Edit) return false;
    auto scene = std::make_unique<EmptyScene>();
    SceneSerializer serializer(*scene);
    if (!serializer.DeSerialize(path)) return false;
    SceneManager::GetInstance().SetScene(std::move(scene));
    SceneManager::GetInstance().ChangeScene();
    m_SelectedEntity = entt::null;
    m_ScenePath = path;
    m_History.clear();
    RecordHistory();
    return true;
}

void FINALITY::EditorLayer::SaveScene()
{
    Scene* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene) return;
    if (m_ScenePath.empty())
    {
        m_ScenePath = FileDialogs::SaveFile(
            "Finality Scene (*.fscene)\0*.fscene\0All Files (*.*)\0*.*\0",
            "Untitled.fscene");
    }
    if (m_ScenePath.empty()) return;
    if (std::filesystem::path(m_ScenePath).extension() != ".fscene")
        m_ScenePath += ".fscene";
    SceneSerializer(*scene, std::filesystem::path(m_ScenePath).stem().string()).Serialize(m_ScenePath);
}

void FINALITY::EditorLayer::RecordHistory()
{
    if (m_State != EditorState::Edit) return;
    Scene* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene) return;
    const std::string snapshot = SceneSerializer(*scene).SerializeToString();
    if (!m_History.empty() && m_History[m_HistoryIndex] == snapshot) return;
    if (!m_History.empty()) m_History.erase(m_History.begin() + m_HistoryIndex + 1, m_History.end());
    m_History.push_back(snapshot);
    if (m_History.size() > 64) m_History.erase(m_History.begin());
    m_HistoryIndex = m_History.size() - 1;
}

void FINALITY::EditorLayer::RestoreHistory(size_t index)
{
    if (index >= m_History.size() || m_State != EditorState::Edit) return;
    Scene* original = SceneManager::GetInstance().GetActiveScene();
    auto restored = std::make_unique<EmptyScene>();
    if (!SceneSerializer(*restored).DeSerializeFromString(m_History[index])) return;
    if (original) original->CopyScriptsTo(*restored);
    SceneManager::GetInstance().SetScene(std::move(restored));
    SceneManager::GetInstance().ChangeScene();
    m_SelectedEntity = entt::null;
    m_HistoryIndex = index;
}

void FINALITY::EditorLayer::OnPlay()
{
    if (m_State != EditorState::Edit) return;
    Scene* editor = SceneManager::GetInstance().GetActiveScene();
    if (!editor) return;
    auto runtime = std::make_unique<EmptyScene>();
    if (!SceneSerializer(*runtime).DeSerializeFromString(SceneSerializer(*editor).SerializeToString())) return;
    editor->CopyScriptsTo(*runtime);
    if (SceneManager::GetInstance().EnterRuntime(std::move(runtime)))
    {
        m_State = EditorState::Play;
        m_SelectedEntity = entt::null;
    }
}

void FINALITY::EditorLayer::OnPause()
{
    m_State = EditorState::Paused;
    SceneManager::GetInstance().SetSimulationEnabled(false);
}

void FINALITY::EditorLayer::OnResume()
{
    m_State = EditorState::Play;
    SceneManager::GetInstance().SetSimulationEnabled(true);
}

void FINALITY::EditorLayer::OnStop()
{
    SceneManager::GetInstance().ExitRuntime();
    Renderer::SetActiveCamera(nullptr);
    m_State = EditorState::Edit;
    m_SelectedEntity = entt::null;
}

void FINALITY::EditorLayer::DrawContentBrowser(entt::entity selectedEntityHandle)
{
    ImGUIPanel panel("Content Browser");
    namespace fs = std::filesystem;
    fs::path current(m_ContentDirectory);
    if (!fs::exists(current))
    {
        current = fs::exists("assets") ? fs::path("assets") : fs::path("SandboxFinality/Assets");
        if (!fs::exists(current)) return;
        m_ContentDirectory = current.string();
    }
    if (panel.Button("Up") && current.has_parent_path())
        m_ContentDirectory = current.parent_path().string();
    panel.SameLine();
    panel.Text(current.generic_string());
    std::error_code error;
    for (fs::directory_iterator it(current, fs::directory_options::skip_permission_denied, error), end;
        it != end && !error; it.increment(error))
    {
        const auto& entry = *it;
        const std::string name = entry.path().filename().string();
        const std::string label = (entry.is_directory() ? "[Folder] " : "[File] ") + name;
        if (!panel.Button(label)) continue;
        if (entry.is_directory())
        {
            m_ContentDirectory = entry.path().string();
            break;
        }
        if (m_State != EditorState::Edit) continue;
        if (entry.path().extension() == ".fscene")
        {
            OpenScene(entry.path().string());
            break;
        }
        if (entry.path().extension() == ".fmat")
        {
            auto* scene = SceneManager::GetInstance().GetActiveScene();
            if (!scene || !scene->GetRegistry().valid(selectedEntityHandle)) continue;
            auto asset = AssetManager::CreateAsset<MaterialAsset>(
                entry.path().generic_string(), Renderer::GetDevice());
            if (!asset) continue;
            auto& registry = scene->GetRegistry();
            auto& component = registry.get_or_emplace<MaterialComponent>(selectedEntityHandle);
            component.MaterialIdentifier = entry.path().generic_string();
            component.MaterialInstance = asset->CreateInstance();
        }
    }
}

void FINALITY::EditorLayer::DrawGizmo(entt::entity selectedEntityHandle)
{
    if (m_State != EditorState::Edit || selectedEntityHandle == entt::null)
        return;

    Scene* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene || !scene->GetRegistry().valid(selectedEntityHandle))
        return;

    auto* transform = scene->GetRegistry().try_get<TransformComponent>(selectedEntityHandle);
    if (!transform)
        return;

    Camera* camera = m_EditorCameraScene->GetCamera();
    if (!camera)
        return;

    // Called directly after ImGUIPanel::Image() inside DrawSceneView().
    // ImGuizmoBridge executes in FinalityEngine.dll, where GImGui is initialized.
    glm::mat4 world = scene->GetWorldTransform(selectedEntityHandle);
    if (!ImGuizmoBridge::Manipulate(
        camera->GetViewMatrix(), camera->GetProjection(),
        m_GizmoOperation, world))
        return;

    if (auto* relation = scene->GetRegistry().try_get<RelationshipComponent>(selectedEntityHandle))
    {
        if (relation->Parent != entt::null && scene->GetRegistry().valid(relation->Parent))
            world = glm::inverse(scene->GetWorldTransform(relation->Parent)) * world;
    }

    ImGuizmoBridge::DecomposeTransform(
        world, transform->Position, transform->Rotation, transform->Scale);
    transform->Matrix = transform->GetTransformMatrix();
}

void FINALITY::EditorLayer::Shutdown()
{
    Renderer::SetFrustumCullingEnabled(true);
    Renderer::SetDirectRenderingEnabled(true);

    FINALITY::RenderDevice* device = &Application::Get().GetDevice();
    device->WaitIdle();

    if (m_EditorCameraScene)
    {
        m_EditorCameraScene->OnDestroy();
        m_EditorCameraScene.reset();
    }

    device->SetGameRenderTarget(nullptr);

    if (m_SceneTarget) { device->DestroyRenderTargetGpuResources(*m_SceneTarget); m_SceneTarget.reset(); }
    if (m_GameTarget) { device->DestroyRenderTargetGpuResources(*m_GameTarget); m_GameTarget.reset(); }
}
