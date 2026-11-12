#include "EditorLayer.h"
#include <ImGUI/AssetPreview.h>
#include <imgui_internal.h>
#include <fstream>
#include <algorithm>
#include "Panels/EntityInspector.h"
#include "ExternalTools.h"

namespace
{
    std::filesystem::path RecentPath()
    {
        if (const auto* local = std::getenv("LOCALAPPDATA")) return std::filesystem::path(local) / "Finality/RecentProjects.txt";
        return std::filesystem::current_path() / ".Finality/RecentProjects.txt";
    }
}

void FINALITY::EditorLayer::SetupLayout()
{
    if (m_LayoutReady) return;
    m_LayoutReady = true;
    const ImGuiID root = 0xF1A117;
    const auto* node = ImGui::DockBuilderGetNode(root);
    if (node && node->IsSplitNode()) return;
    ImGui::DockBuilderRemoveNode(root);
    ImGui::DockBuilderAddNode(root, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(root, ImGui::GetMainViewport()->WorkSize);
    auto center = root;
    const float toolbarRatio = std::clamp(64.0f / ImGui::GetMainViewport()->WorkSize.y, 0.05f, 0.14f);
    const auto top = ImGui::DockBuilderSplitNode(center, ImGuiDir_Up, toolbarRatio, nullptr, &center);
    const auto right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
    const auto bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.28f, nullptr, &center);
    const auto left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.22f, nullptr, &center);
    ImGui::DockBuilderDockWindow("Hierarchy", left);
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Scene", center);
    ImGui::DockBuilderDockWindow("Game", center);
    ImGui::DockBuilderDockWindow("Content Browser", bottom);
    ImGui::DockBuilderDockWindow("Console", bottom);
    ImGui::DockBuilderDockWindow("Performance Overlay", bottom);
    ImGui::DockBuilderDockWindow("Toolbar", top);
    ImGui::DockBuilderFinish(root);
}

bool FINALITY::EditorLayer::OpenProject(const std::string& path)
{
    if (m_State != EditorState::Edit) return false;
    Project next;
    if (!next.Open(path)) { m_ProjectError = "Project could not be opened. Check its format and location."; return false; }
    m_AssetPicker.Close();
    m_AssetInspector.Clear();
    ClearInspectorClipboard();
    m_SelectedAsset = 0;
    Renderer::GetDevice()->WaitIdle();
    SceneManager::GetInstance().SetScene(std::make_unique<EmptyScene>());
    SceneManager::GetInstance().ChangeScene();
    m_ProjectScripts.Close();
    AssetPreview::Clear();
    AssetManager::Shutdown();
    AssetManager::Initialize();
    m_Project = std::move(next);
    ExternalTools::OpenProject(m_Project.GetDirectory(), m_Project.Settings.ScriptDirectory);
    if (!m_LayoutPath.empty()) ImGui::SaveIniSettingsToDisk(m_LayoutPath.c_str());
    m_LayoutPath = (m_Project.GetDirectory() / "Settings/EditorLayout.ini").string();
    std::filesystem::create_directories(std::filesystem::path(m_LayoutPath).parent_path());
    ImGui::GetIO().IniFilename = m_LayoutPath.c_str();
    if (std::filesystem::exists(m_LayoutPath)) ImGui::LoadIniSettingsFromDisk(m_LayoutPath.c_str());
    ModelAsset::Configure(m_EngineAssets,m_Project.Settings.Models);
    AssetRegistry::Open(m_Project.GetDirectory());
    m_ProjectScripts.Open(m_Project,m_EngineAssets);
    m_SettingsInitialized = false;
    ApplyProjectSettings();
    m_ContentDirectory = m_Project.GetAssetsDirectory().string();
    m_SelectedEntity = entt::null;
    m_History.clear();
    m_ScenePath.clear();
    m_ShowProjectManager = false;
    m_ProjectError.clear();
    if (!m_Project.StartupScene.empty() || std::filesystem::exists(m_Project.GetDirectory() / "Settings/LastScene.txt"))
    {
        std::string lastScene;
        std::ifstream(m_Project.GetDirectory() / "Settings/LastScene.txt") >> std::quoted(lastScene);
        auto startup = m_Project.GetDirectory() / m_Project.StartupScene;
        if (!lastScene.empty() && std::filesystem::is_regular_file(lastScene)) startup = lastScene;
        if (!OpenScene(startup.string())) { NewScene(); FI_CLIENT_WARN("Startup scene is missing; opened an empty scene"); }
    }
    else NewScene();
    Application::Get().GetWindow().SetWindowTitle("Finality - " + m_Project.GetName());
    m_RecentProjects.erase(std::remove(m_RecentProjects.begin(), m_RecentProjects.end(), m_Project.GetPath().string()), m_RecentProjects.end());
    m_RecentProjects.insert(m_RecentProjects.begin(), m_Project.GetPath().string());
    if (m_RecentProjects.size() > 12) m_RecentProjects.resize(12);
    std::filesystem::create_directories(RecentPath().parent_path());
    std::ofstream recent(RecentPath());
    for (const auto& entry : m_RecentProjects) recent << entry << '\n';
    return true;
}

void FINALITY::EditorLayer::DrawProjectManager()
{
    static bool loadedRecent = false;
    if (!loadedRecent)
    {
        loadedRecent = true;
        std::ifstream recent(RecentPath());
        std::string entry;
        while (std::getline(recent, entry)) if (!entry.empty()) m_RecentProjects.push_back(entry);
    }
    if (!m_ShowProjectManager) return;
    ImGui::SetNextWindowSize({620,470}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Finality Project Manager", m_Project.IsOpen() ? &m_ShowProjectManager : nullptr);
    ImGui::TextUnformatted("Create a project or open an existing .fproj.");
    ImGui::Separator();
    EditText("Project Name", m_ProjectName);
    EditText("Location", m_ProjectLocation);
    if (ImGui::Button("Choose Location..."))
    {
        const auto path = FileDialogs::OpenFolder();
        if (!path.empty()) m_ProjectLocation = path;
    }
    ImGui::Checkbox("Include extra starter assets", &m_UseTemplate);
    if (ImGui::Button("Create Project"))
    {
        Project created;
        if (created.Create(m_ProjectLocation, m_ProjectName, m_EngineAssets, m_UseTemplate)) OpenProject(created.GetPath().string());
        else m_ProjectError = "Creation failed. Use a valid name and an empty writable directory.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Project..."))
    {
        const auto path = FileDialogs::OpenFile("Finality Project (*.fproj)\0*.fproj\0\0");
        if (!path.empty()) OpenProject(path);
    }
    if (!m_ProjectError.empty()) ImGui::TextWrapped("%s", m_ProjectError.c_str());
    ImGui::Separator();
    ImGui::TextUnformatted("Recent Projects");
    std::string open;
    for (const auto& path : m_RecentProjects)
        if (ImGui::Selectable(path.c_str())) { open = path; break; }
    if (!open.empty()) OpenProject(open);
    ImGui::End();
}

void FINALITY::EditorLayer::CreatePrimitive(const std::string& name, const std::string& meshReference)
{
    auto* scene = SceneManager::GetInstance().GetActiveScene();
    if (!scene || m_State != EditorState::Edit) return;
    if (!meshReference.empty())
    {
        try { m_SelectedEntity = ModelAsset::Instantiate(*scene,meshReference).GetHandle(); m_SelectedAsset = 0; }
        catch (const std::exception& error) { FI_CLIENT_ERROR("Cannot instantiate model: {}",error.what()); }
        return;
    }
    auto entity = scene->CreateEntity(name);
    if (name == "Camera") entity.AddComponent<CameraComponent>().IsPrimary = false;
    else if (name != "Empty Entity")
    {
        std::string primitive = name;
        std::transform(primitive.begin(), primitive.end(), primitive.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const std::string identifier = meshReference.empty() ? "builtin://" + primitive : meshReference;
        entity.AddComponent<MeshComponent>(Mesh::Create(identifier), identifier);
        const auto materialPath = m_Project.GetAssetsDirectory() / "materials/Default.fmat";
        const auto reference = AssetRegistry::Reference(materialPath);
        if (auto material = AssetManager::CreateAsset<MaterialAsset>(reference, Renderer::GetDevice()))
            entity.AddComponent<MaterialComponent>(material->CreateInstance(), reference);
    }
    m_SelectedEntity = entity.GetHandle();
    m_SelectedAsset = 0;
}

void FINALITY::EditorLayer::DrawCreateMenu()
{
    for (const char* name : {"Empty Entity", "Camera", "Cube", "Sphere", "Capsule", "Cylinder", "Plane", "Quad"})
        if (ImGui::MenuItem(name)) CreatePrimitive(name);
}
