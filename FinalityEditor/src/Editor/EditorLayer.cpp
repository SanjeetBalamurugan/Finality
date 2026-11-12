
#include "EditorLayer.h"
#include "Panels/EntityInspector.h"
#include <Util/Helpers.h>
#include <filesystem>
#include <Assets/MaterialAsset.h>
#include <ImGUI/ImGuizmoBridge.h>
#include <ImGUI/AssetPreview.h>
#include <ImGUI/EditorTheme.h>

#include <imgui_internal.h>
#include "ExternalTools.h"
#include "Prefab.h"
#include "ImportProgress.h"

void FINALITY::EditorLayer::Init()
{
    const auto* userDirectory = std::getenv("USERPROFILE");
    m_ProjectLocation = ((userDirectory ? std::filesystem::path(userDirectory) / "Documents" : std::filesystem::current_path()) / "Finality Projects").string();
    if (const auto* localDirectory = std::getenv("LOCALAPPDATA"))
    {
        const auto layout = std::filesystem::path(localDirectory) / "Finality/EditorLayout.ini";
        std::filesystem::create_directories(layout.parent_path());
        m_LayoutPath = layout.string();
        ImGui::GetIO().IniFilename = m_LayoutPath.c_str();
        if (std::filesystem::exists(layout)) ImGui::LoadIniSettingsFromDisk(m_LayoutPath.c_str());
    }
    FINALITY::RenderDevice* device = &Application::Get().GetDevice();

    m_SceneTarget = device->CreateRenderTarget(1280, 720);
    m_GameTarget = device->CreateRenderTarget(1280, 720);

    m_SceneTarget->ClearColor = glm::vec4(0.075f, 0.09f, 0.11f, 1.0f);
    InitializeSceneTools();
    device->SetSwapchainClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    device->SetGameRenderTarget(m_GameTarget);

    m_EditorCameraScene = std::make_unique<EditorCameraScene>();
    m_EditorCameraScene->OnInit();

    Renderer::SetDirectRenderingEnabled(false);
    Renderer::SetFrustumCullingEnabled(false);
    SceneManager::GetInstance().SetSimulationEnabled(false);
    m_LastLogTime = std::chrono::steady_clock::now();
    if (!m_InitialProject.empty()) OpenProject(m_InitialProject);
}

void FINALITY::EditorLayer::OnUpdate(float deltaTime)
{
    ExternalTools::Update();
    ImportProgress::Update();
    for (const auto& path : Application::Get().GetWindow().ConsumeDroppedPaths())
        if (m_Project.IsOpen() && m_State == EditorState::Edit)
        {
            AssetPreview::Clear();
            ImportProgress::Import(std::filesystem::u8path(path), std::filesystem::path(m_ContentDirectory).lexically_relative(m_Project.GetDirectory()));
        }
    if (m_ThemePending)
    {
        m_ThemePending = false;
        if (!EditorTheme::Apply(m_Project.Settings.Theme, m_EngineAssets, m_Project.GetDirectory(), m_ThemeMessage))
            FI_CLIENT_ERROR("Theme: {}", m_ThemeMessage);
    }
    AssetPreview::Update();
    if (m_Project.IsOpen())
    {
        m_ProjectScripts.Update(m_Project,SceneManager::GetInstance().GetActiveScene(),m_State == EditorState::Edit);
        m_AssetScanClock += deltaTime;
        if (m_Project.Settings.AutoReimport && m_AssetScanClock >= m_Project.Settings.AssetScanInterval && m_State == EditorState::Edit)
        {
            m_AssetScanClock = 0;
            AssetRegistry::Scan();
        }
    }
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
    SetupLayout();
    ImportProgress::Draw(m_ProjectScripts.IsBuilding() || ExternalTools::SolutionGeneration.valid(),
        ExternalTools::SolutionGeneration.valid() ? ExternalTools::Status : m_ProjectScripts.GetStatus());
    DrawMenuBar();
    DrawProjectManager();
    if (!m_Project.IsOpen()) return;
    if (m_ShowToolbar) DrawToolbar();
    if (m_ShowScene) DrawSceneView();
    else m_SceneViewActive = false;
    if (m_ShowGame) DrawGameView();
    if (m_ShowConsole) FINALITY::DrawConsole(&m_ShowConsole);

    if (m_ShowPerformance) FINALITY::DrawPerformancePanel(m_UiFps, m_HistoryOffset, m_FrameTimeHistory, m_UiAvgFrameTimeMs, &m_ShowPerformance);

    Scene* activeScene = SceneManager::GetInstance().GetActiveScene();
    if (!activeScene) activeScene = m_EditorCameraScene.get();
    if (!activeScene->GetRegistry().valid(m_SelectedEntity))
        m_SelectedEntity = entt::null;
    ImGui::BeginDisabled(m_State != EditorState::Edit);
    if (m_ShowHierarchy) DrawHierarchy();
    if (m_ShowInspector)
    {
        if (m_SelectedAsset) m_AssetInspector.Draw(m_SelectedAsset, m_AssetPicker, &m_ShowInspector);
        else DrawEntityInspector(activeScene, m_SelectedEntity, m_AssetPicker, &m_ShowInspector);
    }
    if (m_ShowContentBrowser) DrawContentBrowser(m_SelectedEntity);
    DrawProjectSettings();
    ImGui::EndDisabled();
    m_AssetPicker.Draw();
    if (!m_FocusWindow.empty())
    {
        ImGui::SetWindowFocus(m_FocusWindow.c_str());
        m_FocusWindow.clear();
    }
    if (m_State == EditorState::Edit)
        RecordHistory();
    if (!ImGui::GetIO().WantTextInput && m_State == EditorState::Edit)
    {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveScene();
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && m_HistoryIndex > 0) RestoreHistory(m_HistoryIndex - 1);
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) && m_HistoryIndex + 1 < m_History.size()) RestoreHistory(m_HistoryIndex + 1);
        if (m_SceneViewActive && !ImGui::GetIO().KeyCtrl && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_GizmoOperation = 0;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_GizmoOperation = 1;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_GizmoOperation = 2;
        }
    }
}

void FINALITY::EditorLayer::DrawToolbar()
{
    //FINALITY::ImGUIPanel panel("Toolbar", nullptr, FINALITY::PanelFlags_NoTitleBar | FINALITY::PanelFlags_NoResize | FINALITY::PanelFlags_NoMove);
    FINALITY::ImGUIPanel panel("Toolbar", &m_ShowToolbar);

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
    panel.Checkbox("Local", &m_LocalGizmo);
    panel.SameLine();
    if (panel.Checkbox("Grid", &m_ShowGrid)) m_Project.Settings.Grid = m_ShowGrid;
}

void FINALITY::EditorLayer::DrawSceneView()
{
    FINALITY::ImGUIPanel panel("Scene", &m_ShowScene);

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
    auto sceneQueue = Renderer::GetRenderQueue();
    AppendSceneGrid(sceneQueue, *editorCam);
    device->RenderSceneToTarget(m_SceneTarget, sceneQueue);

    void* textureHandle = device->GetRenderTargetTextureHandle(m_SceneTarget);
    panel.Image(textureHandle, panelSize);
    const auto imageMinimum = ImGui::GetItemRectMin();
    const auto imageMaximum = ImGui::GetItemRectMax();
    const bool select = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (m_State == EditorState::Edit && ImGui::BeginDragDropTarget())
    {
        if (const auto* payload = ImGui::AcceptDragDropPayload("FinalityAsset"))
        {
            if (payload->DataSize == sizeof(uint64_t))
                if (const auto* record = AssetRegistry::Find(*static_cast<const uint64_t*>(payload->Data)); record)
                {
                    if (record->Type == ProjectAssetType::Model)
                        CreatePrimitive(std::filesystem::path(record->Path).stem().string(), "asset://" + std::to_string(record->ID));
                    if (record->Type == ProjectAssetType::Prefab)
                        if (auto* scene = SceneManager::GetInstance().GetActiveScene())
                            m_SelectedEntity = InstantiatePrefab(*scene, AssetRegistry::Resolve("asset://" + std::to_string(record->ID))).GetHandle();
                }
        }
        ImGui::EndDragDropTarget();
    }
    DrawGizmo(m_SelectedEntity);
    if (select && m_State == EditorState::Edit && !ImGui::GetDragDropPayload() &&
        (m_SelectedEntity == entt::null || !ImGuizmoBridge::IsActive())) PickSceneEntity(imageMinimum, imageMaximum);
}

void FINALITY::EditorLayer::DrawGameView()
{
    FINALITY::ImGUIPanel panel("Game", &m_ShowGame);

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
        if (ImGUIPanel::MenuItem("Project Manager") && m_State == EditorState::Edit) m_ShowProjectManager = true;
        if (ImGUIPanel::MenuItem("Save Project")) m_Project.Save();
        if (ImGUIPanel::MenuItem("Project Settings")) m_ShowProjectSettings = true;
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
    if (ImGUIPanel::BeginMenu("Create"))
    {
        if (m_Project.IsOpen() && m_State == EditorState::Edit) DrawCreateMenu();
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
    if (ImGUIPanel::BeginMenu("Assets"))
    {
        if (ImGUIPanel::MenuItem("Recompile Shaders")) AssetManager::ReloadMaterialShaders(true);
        ImGUIPanel::EndMenu();
    }
    if (ImGui::BeginMenu("Window"))
    {
        auto panel = [this](const char* name, bool& visible)
        {
            if (ImGui::MenuItem(name, nullptr, &visible) && visible) m_FocusWindow = name;
        };
        panel("Scene", m_ShowScene);
        panel("Game", m_ShowGame);
        panel("Hierarchy", m_ShowHierarchy);
        panel("Inspector", m_ShowInspector);
        panel("Content Browser", m_ShowContentBrowser);
        panel("Console", m_ShowConsole);
        panel("Toolbar", m_ShowToolbar);
        panel("Performance Overlay", m_ShowPerformance);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout"))
        {
            ImGui::ClearIniSettings();
            ImGui::DockBuilderRemoveNode(0xF1A117);
            m_LayoutReady = false;
            m_ShowScene = m_ShowGame = m_ShowHierarchy = m_ShowInspector = true;
            m_ShowContentBrowser = m_ShowConsole = m_ShowToolbar = true;
        }
        if (ImGui::MenuItem("Project Settings")) m_ShowProjectSettings = true;
        if (ImGui::MenuItem("Project Manager") && m_State == EditorState::Edit) m_ShowProjectManager = true;
        ImGui::EndMenu();
    }
    ImGUIPanel::EndMainMenuBar();
}

void FINALITY::EditorLayer::NewScene()
{
    if (m_State != EditorState::Edit) return;
    auto scene = std::make_unique<EmptyScene>();
    auto camera = scene->CreateEntity("Main Camera");
    camera.AddComponent<CameraComponent>(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera.GetComponent<TransformComponent>().Position = {0,1.5f,5};
    camera.GetComponent<TransformComponent>().Rotation = {-15,0,0};
    SceneManager::GetInstance().SetScene(std::move(scene));
    SceneManager::GetInstance().ChangeScene();
    m_SelectedEntity = entt::null;
    m_ScenePath.clear();
    m_SelectedAsset = 0;
    m_History.clear();
    RecordHistory();
}

bool FINALITY::EditorLayer::OpenScene(const std::string& path)
{
    if (m_State != EditorState::Edit) return false;
    auto scene = std::make_unique<EmptyScene>();
    SceneSerializer serializer(*scene);
    if (!serializer.DeSerialize(path)) return false;
    m_AssetPicker.Close();
    SceneManager::GetInstance().SetScene(std::move(scene));
    SceneManager::GetInstance().ChangeScene();
    m_SelectedEntity = entt::null;
    m_ScenePath = path;
    if (m_Project.IsOpen()) std::ofstream(m_Project.GetDirectory() / "Settings/LastScene.txt") << std::quoted(path);
    m_SelectedAsset = 0;
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
    if (m_Project.IsOpen()) std::ofstream(m_Project.GetDirectory() / "Settings/LastScene.txt") << std::quoted(m_ScenePath);
    AssetRegistry::Scan();
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
    m_SelectedAsset = 0;
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
        m_GizmoOperation, world, m_LocalGizmo))
        return;

    if (auto* relation = scene->GetRegistry().try_get<RelationshipComponent>(selectedEntityHandle))
    {
        if (relation->Parent != entt::null && scene->GetRegistry().valid(relation->Parent))
            world = glm::inverse(scene->GetWorldTransform(relation->Parent)) * world;
    }

    ImGuizmoBridge::SetLocalTransform(*transform,world);
}

void FINALITY::EditorLayer::Shutdown()
{
    if (!m_LayoutPath.empty()) ImGui::SaveIniSettingsToDisk(m_LayoutPath.c_str());
    ImGui::GetIO().IniFilename = nullptr;
    m_AssetPicker.Close();
    AssetPreview::Clear();
    m_ProjectScripts.Close();
    m_AssetInspector.Clear();
    ClearInspectorClipboard();
    if (m_Project.IsOpen()) { m_Project.Save(); AssetRegistry::Save(); }
    Renderer::SetFrustumCullingEnabled(true);
    Renderer::SetDirectRenderingEnabled(true);

    FINALITY::RenderDevice* device = &Application::Get().GetDevice();
    device->WaitIdle();
    m_GridMaterial.reset();
    m_GridMesh.reset();

    if (m_EditorCameraScene)
    {
        m_EditorCameraScene->OnDestroy();
        m_EditorCameraScene.reset();
    }

    device->SetGameRenderTarget(nullptr);

    if (m_SceneTarget) { device->DestroyRenderTargetGpuResources(*m_SceneTarget); m_SceneTarget.reset(); }
    if (m_GameTarget) { device->DestroyRenderTargetGpuResources(*m_GameTarget); m_GameTarget.reset(); }
}
