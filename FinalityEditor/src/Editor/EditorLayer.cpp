
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
#include "ScenePlacement.h"

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
    m_Session.Tools.Update();
    m_Session.Imports.Update();
    for (const auto& path : Application::Get().GetWindow().ConsumeDroppedPaths())
        if (m_Session.ProjectData.IsOpen() && m_Session.State == EditorState::Edit)
        {
            AssetPreview::Clear();
            const auto source = std::filesystem::u8path(path);
            const auto mouse = ImGui::GetMousePos();
            const bool overScene = m_ShowScene && mouse.x >= m_SceneImageMinimum.x && mouse.y >= m_SceneImageMinimum.y &&
                mouse.x < m_SceneImageMaximum.x && mouse.y < m_SceneImageMaximum.y;
            if (overScene && AssetRegistry::GetType(source) == ProjectAssetType::Model)
            {
                const auto position = SceneDropPosition(*m_EditorCameraScene->GetCamera(),m_SceneImageMinimum,m_SceneImageMaximum,mouse);
                m_Session.Imports.Queue(source.filename().string(),[this,source,position]
                {
                    if (AssetRegistry::Import(source,"Assets"))
                        if (const auto* record = AssetRegistry::FindPath(m_Session.ProjectData.GetAssetsDirectory()/source.filename()))
                            PlaceSceneAsset(record->ID,position);
                });
            }
            else m_Session.Imports.Import(source,std::filesystem::path(m_ContentDirectory).lexically_relative(m_Session.ProjectData.GetDirectory()));
        }
    if (m_ThemePending)
    {
        m_ThemePending = false;
        if (!EditorTheme::Apply(m_Session.ProjectData.Settings.Theme, m_EngineAssets, m_Session.ProjectData.GetDirectory(), m_ThemeMessage))
            FI_CLIENT_ERROR("Theme: {}", m_ThemeMessage);
    }
    AssetPreview::Flush();
    AssetPreview::Update();
    if (m_Session.ProjectData.IsOpen())
    {
        if (!m_BuildPanel.IsBuilding()) m_Session.Scripts.Update(m_Session.ProjectData,SceneManager::GetInstance().GetActiveScene(),m_Session.State == EditorState::Edit);
        if (m_Session.ProjectData.Settings.AutoReimport && m_Session.State == EditorState::Edit &&
            m_Session.AssetChanges.Poll(m_Session.ProjectData.GetAssetsDirectory(),m_Session.ProjectData.Settings.AssetScanInterval)) m_AssetRefreshPending = true;
        if (m_AssetRefreshPending && !m_Session.Imports.IsBusy() && m_Session.State == EditorState::Edit)
        {
            m_AssetRefreshPending = false;
            m_Session.Imports.Queue("Refreshing changed assets",[] { AssetRegistry::Scan(); });
        }
    }
    if (m_CameraCaptured && (!ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::GetIO().AppFocusLost))
    { m_CameraCaptured = false; RenderCommand::SetCursorMode(false); }
    if (!ImGui::GetIO().AppFocusLost && (m_SceneViewActive || m_CameraCaptured))
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
    ImGuizmoBridge::BeginFrame();
    SetupLayout();
    ImGui::DockSpaceOverViewport(0xF1A117, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    m_Session.Imports.Draw(m_Session.Scripts.IsBuilding() || m_Session.Tools.SolutionGeneration.valid(),
        m_Session.Tools.SolutionGeneration.valid() ? m_Session.Tools.Status : m_Session.Scripts.GetStatus());
    DrawMenuBar();
    DrawProjectManager();
    m_BuildPanel.Draw(m_Session.ProjectData,m_EngineAssets,m_Session.ScenePath,m_Session.State==EditorState::Edit && !m_Session.Scripts.IsBuilding() && !m_Session.Imports.IsBusy());
    if(m_BuildPanel.OpenPlayerSettings){m_BuildPanel.OpenPlayerSettings=false;m_ShowProjectSettings=true;m_SettingsCategory=7;}
    if (!m_Session.ProjectData.IsOpen()) return;
    if (m_ShowToolbar) DrawToolbar();
    if (m_ShowScene) DrawSceneView();
    else m_SceneViewActive = false;
    if (m_ShowGame) DrawGameView();
    if (m_ShowConsole) FINALITY::DrawConsole(&m_ShowConsole);

    if (m_ShowPerformance) FINALITY::DrawPerformancePanel(m_UiFps, m_HistoryOffset, m_FrameTimeHistory, m_UiAvgFrameTimeMs, m_SceneTarget.get(), m_GameTarget.get(), &m_ShowPerformance);

    Scene* activeScene = SceneManager::GetInstance().GetActiveScene();
    if (!activeScene) activeScene = m_EditorCameraScene.get();
    if (!activeScene->GetRegistry().valid(m_Session.SelectedEntity))
        m_Session.SelectedEntity = entt::null;
    ImGui::BeginDisabled(m_Session.State != EditorState::Edit);
    if (m_ShowHierarchy) DrawHierarchy();
    if (m_ShowInspector)
    {
        if (m_Session.SelectedAsset) m_AssetInspector.Draw(m_Session.SelectedAsset, m_AssetPicker, m_Session.Tools, &m_ShowInspector);
        else DrawEntityInspector(activeScene, m_Session.SelectedEntity, m_AssetPicker, m_Session.Tools, &m_ShowInspector);
    }
    m_ContentBrowserFocused = m_ContentBrowserVisible = false;
    if (m_ShowContentBrowser) DrawContentBrowser(m_Session.SelectedEntity);
    DrawProjectSettings();
    ImGui::EndDisabled();
    m_AssetPicker.Draw();
    HandleShortcuts();
    DrawEntityCommandDialogs();
    if (!m_FocusWindow.empty())
    {
        ImGui::SetWindowFocus(m_FocusWindow.c_str());
        m_FocusWindow.clear();
    }
    if (m_Session.State == EditorState::Edit && m_Session.Checkpoint.Update(
        ImGui::IsAnyItemActive() || ImGui::IsMouseDown(ImGuiMouseButton_Left),
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsKeyReleased(ImGuiKey_Enter) || ImGui::IsKeyReleased(ImGuiKey_Delete)))
        RecordHistory();
}

void FINALITY::EditorLayer::DrawToolbar()
{
    //FINALITY::ImGUIPanel panel("Toolbar", nullptr, FINALITY::PanelFlags_NoTitleBar | FINALITY::PanelFlags_NoResize | FINALITY::PanelFlags_NoMove);
    FINALITY::ImGUIPanel panel("Toolbar", &m_ShowToolbar);

    if (panel.Button("Play") && m_Session.State == EditorState::Edit)
    {
        OnPlay();
    }
    panel.SameLine();

    std::string_view pauseLabel = (m_Session.State == EditorState::Paused) ? "Resume" : "Pause";
    if (panel.Button(pauseLabel))
    {
        if (m_Session.State == EditorState::Play) OnPause();
        else if (m_Session.State == EditorState::Paused) OnResume();
    }
    panel.SameLine();

    if (panel.Button("Stop") && m_Session.State != EditorState::Edit)
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
    if (panel.Checkbox("Grid", &m_ShowGrid)) m_Session.ProjectData.Settings.Grid = m_ShowGrid;
    panel.SameLine();
    ImGui::Checkbox("Snap", &m_SnapToGrid);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap movement to the world grid. One unit is one metre.");
    panel.SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::DragFloat("Step (m)",&m_SnapDistance,0.1f,0.001f,10000.0f,"%.3g",ImGuiSliderFlags_AlwaysClamp);
}

void FINALITY::EditorLayer::DrawMenuBar()
{
    if (!ImGUIPanel::BeginMainMenuBar()) return;
    if (ImGUIPanel::BeginMenu("File"))
    {
        if (ImGUIPanel::MenuItem("Project Manager") && m_Session.State == EditorState::Edit) m_ShowProjectManager = true;
        if (ImGUIPanel::MenuItem("Save Project")) m_Session.ProjectData.Save();
        if (ImGUIPanel::MenuItem("Project Settings")) m_ShowProjectSettings = true;
        if (ImGui::MenuItem("Build Settings...","Ctrl+Shift+B")) m_BuildPanel.Visible=true;
        if (ImGui::MenuItem("Build And Run","Ctrl+B",false,m_Session.ProjectData.IsOpen() && m_Session.State==EditorState::Edit && !m_Session.Scripts.IsBuilding() && !m_Session.Imports.IsBusy())) m_BuildPanel.Start(m_Session.ProjectData,m_EngineAssets,true);
        if (ImGui::MenuItem("New Scene",m_Session.Preferences.Label(EditorAction::NewScene).c_str()) && m_Session.State == EditorState::Edit) NewScene();
        if (ImGui::MenuItem("Save Scene",m_Session.Preferences.Label(EditorAction::Save).c_str()) && m_Session.State == EditorState::Edit) SaveScene();
        if (ImGui::MenuItem("Save Scene As...",m_Session.Preferences.Label(EditorAction::SaveAs).c_str()) && m_Session.State == EditorState::Edit) SaveScene(true);
        if (ImGui::MenuItem("Open Scene",m_Session.Preferences.Label(EditorAction::OpenScene).c_str()) && m_Session.State == EditorState::Edit)
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
        if (m_Session.ProjectData.IsOpen() && m_Session.State == EditorState::Edit) DrawCreateMenu();
        ImGUIPanel::EndMenu();
    }
    if (ImGUIPanel::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Preferences")) { m_ShowProjectSettings = true; m_SettingsCategory = 6; }
        if (ImGui::MenuItem("Undo",m_Session.Preferences.Label(EditorAction::Undo).c_str()) && m_Session.State == EditorState::Edit && m_Session.HistoryIndex > 0)
            RestoreHistory(m_Session.HistoryIndex - 1);
        if (ImGui::MenuItem("Redo",m_Session.Preferences.Label(EditorAction::Redo).c_str()) && m_Session.State == EditorState::Edit && m_Session.HistoryIndex + 1 < m_Session.History.size())
            RestoreHistory(m_Session.HistoryIndex + 1);
        ImGui::Separator();
        for (auto action : {EditorAction::Copy,EditorAction::Cut,EditorAction::Paste,EditorAction::Duplicate,EditorAction::Delete,EditorAction::Rename})
        {
            const auto& definition = EditorPreferences::Definitions()[static_cast<size_t>(action)];
            const bool available = m_Session.State == EditorState::Edit && (action == EditorAction::Paste ? !m_EntityClipboard.empty() : m_Session.SelectedEntity != entt::null);
            if (ImGui::MenuItem(definition.Label,m_Session.Preferences.Label(action).c_str(),false,available)) ExecuteEntityCommand(action);
        }
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
        if (ImGui::MenuItem("Preferences")) { m_ShowProjectSettings = true; m_SettingsCategory = 6; }
        if (ImGui::MenuItem("Project Manager") && m_Session.State == EditorState::Edit) m_ShowProjectManager = true;
        ImGui::EndMenu();
    }
    ImGUIPanel::EndMainMenuBar();
}

void FINALITY::EditorLayer::NewScene()
{
    if (m_Session.State != EditorState::Edit) return;
    auto scene = std::make_unique<EmptyScene>();
    auto camera = scene->CreateEntity("Main Camera");
    camera.AddComponent<CameraComponent>(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera.GetComponent<TransformComponent>().Position = {0,1.5f,5};
    camera.GetComponent<TransformComponent>().Rotation = {-15,0,0};
    SceneManager::GetInstance().SetScene(std::move(scene));
    SceneManager::GetInstance().ChangeScene();
    m_Session.SelectedEntity = entt::null;
    m_Session.ScenePath.clear();
    m_Session.SelectedAsset = 0;
    m_Session.History.clear();
    RecordHistory();
}

bool FINALITY::EditorLayer::OpenScene(const std::string& path)
{
    if (m_Session.State != EditorState::Edit) return false;
    auto scene = std::make_unique<EmptyScene>();
    SceneSerializer serializer(*scene);
    if (!serializer.DeSerialize(path)) return false;
    m_AssetPicker.Close();
    SceneManager::GetInstance().SetScene(std::move(scene));
    SceneManager::GetInstance().ChangeScene();
    m_Session.SelectedEntity = entt::null;
    m_Session.ScenePath = path;
    if (m_Session.ProjectData.IsOpen()) std::ofstream(m_Session.ProjectData.GetDirectory() / "Settings/LastScene.txt") << std::quoted(path);
    m_Session.SelectedAsset = 0;
    m_Session.History.clear();
    RecordHistory();
    return true;
}

void FINALITY::EditorLayer::SaveScene(bool saveAs)
{
    Scene* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene) return;
    if (m_Session.ScenePath.empty() || saveAs)
    {
        const auto path = FileDialogs::SaveFile(
            "Finality Scene (*.fscene)\0*.fscene\0All Files (*.*)\0*.*\0",
            "Untitled.fscene");
        if (path.empty()) return;
        m_Session.ScenePath = path;
    }
    if (m_Session.ScenePath.empty()) return;
    if (std::filesystem::path(m_Session.ScenePath).extension() != ".fscene")
        m_Session.ScenePath += ".fscene";
    SceneSerializer(*scene, std::filesystem::path(m_Session.ScenePath).stem().string()).Serialize(m_Session.ScenePath);
    if (m_Session.ProjectData.IsOpen()) std::ofstream(m_Session.ProjectData.GetDirectory() / "Settings/LastScene.txt") << std::quoted(m_Session.ScenePath);
    AssetRegistry::Scan();
}

void FINALITY::EditorLayer::RecordHistory()
{
    m_Session.Checkpoint.Completed();
    if (m_Session.State != EditorState::Edit) return;
    Scene* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene) return;
    const std::string snapshot = SceneSerializer(*scene).SerializeToString();
    if (!m_Session.History.empty() && m_Session.History[m_Session.HistoryIndex] == snapshot) return;
    if (!m_Session.History.empty()) m_Session.History.erase(m_Session.History.begin() + m_Session.HistoryIndex + 1, m_Session.History.end());
    m_Session.History.push_back(snapshot);
    if (m_Session.History.size() > 64) m_Session.History.erase(m_Session.History.begin());
    m_Session.HistoryIndex = m_Session.History.size() - 1;
}

void FINALITY::EditorLayer::RestoreHistory(size_t index)
{
    if (index >= m_Session.History.size() || m_Session.State != EditorState::Edit) return;
    Scene* original = SceneManager::GetInstance().GetActiveScene();
    auto restored = std::make_unique<EmptyScene>();
    if (!SceneSerializer(*restored).DeSerializeFromString(m_Session.History[index])) return;
    if (original) original->CopyScriptsTo(*restored);
    SceneManager::GetInstance().SetScene(std::move(restored));
    SceneManager::GetInstance().ChangeScene();
    m_Session.SelectedEntity = entt::null;
    m_Session.HistoryIndex = index;
    m_Session.SelectedAsset = 0;
}

void FINALITY::EditorLayer::OnPlay()
{
    if (m_Session.State != EditorState::Edit) return;
    Scene* editor = SceneManager::GetInstance().GetActiveScene();
    if (!editor) return;
    auto runtime = std::make_unique<EmptyScene>();
    if (!SceneSerializer(*runtime).DeSerializeFromString(SceneSerializer(*editor).SerializeToString())) return;
    editor->CopyScriptsTo(*runtime);
    if (SceneManager::GetInstance().EnterRuntime(std::move(runtime)))
    {
        m_Session.State = EditorState::Play;
        m_Session.SelectedEntity = entt::null;
    }
}

void FINALITY::EditorLayer::OnPause()
{
    m_Session.State = EditorState::Paused;
    SceneManager::GetInstance().SetSimulationEnabled(false);
}

void FINALITY::EditorLayer::OnResume()
{
    m_Session.State = EditorState::Play;
    SceneManager::GetInstance().SetSimulationEnabled(true);
}

void FINALITY::EditorLayer::OnStop()
{
    SceneManager::GetInstance().ExitRuntime();
    Renderer::SetActiveCamera(nullptr);
    m_Session.State = EditorState::Edit;
    m_Session.SelectedEntity = entt::null;
}

void FINALITY::EditorLayer::DrawGizmo(entt::entity selectedEntityHandle)
{
    if (m_Session.State != EditorState::Edit || selectedEntityHandle == entt::null)
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
    const glm::vec3 previousPosition(world[3]);
    if (!ImGuizmoBridge::Manipulate(
        camera->GetViewMatrix(), camera->GetProjection(),
        m_GizmoOperation, world, m_LocalGizmo))
        return;

    if (m_SnapToGrid && m_GizmoOperation == 0 && std::isfinite(m_SnapDistance) && m_SnapDistance > 0)
        for (int axis = 0; axis < 3; ++axis)
            if (std::abs(world[3][axis] - previousPosition[axis]) > 0.000001f)
                world[3][axis] = std::round(world[3][axis] / m_SnapDistance) * m_SnapDistance;

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
    m_Session.CloseProjectTools();
    m_AssetInspector.Clear();
    ClearInspectorClipboard();
    if (m_Session.ProjectData.IsOpen()) { m_Session.ProjectData.Save(); AssetRegistry::Save(); }
    Renderer::SetFrustumCullingEnabled(true);
    Renderer::SetDirectRenderingEnabled(true);

    FINALITY::RenderDevice* device = &Application::Get().GetDevice();
    device->WaitIdle();
    AssetPreview::Flush();
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
