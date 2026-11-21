#include "EditorLayer.h"
#include <ImGUI/AssetPreview.h>
#include <ImGUI/EditorTheme.h>
#include <algorithm>
#include "ExternalTools.h"

void FINALITY::EditorLayer::ApplyProjectSettings()
{
    m_ThemePending = true;
    const auto& settings = m_Session.ProjectData.Settings;
    m_ShowGrid = settings.Grid;
    m_SceneTarget->ClearColor = settings.SceneBackground;
    m_EditorCameraScene->ConfigureNavigation(settings.CameraSpeed,settings.CameraSprint,settings.CameraSensitivity);
    if (m_GridMaterial)
    {
        m_GridMaterial->SetUniform1f("spacing",settings.GridSpacing);
        m_GridMaterial->SetUniform1f("fadeDistance",settings.GridFadeDistance);
    }
    AssetPreview::Configure(settings.ThumbnailSize,settings.ThumbnailCapacity);
    ModelAsset::Configure(m_EngineAssets,settings.Models);
}

void FINALITY::EditorLayer::DrawProjectSettings()
{
    if (!m_ShowProjectSettings) { m_SettingsInitialized = false; m_ShortcutCapture = -1; return; }
    if (m_SettingsCategory != 6) m_ShortcutCapture = -1;
    if (!m_SettingsInitialized)
    {
        m_SettingsDraft = m_Session.ProjectData.Settings; m_SettingsName = m_Session.ProjectData.GetName(); m_SettingsStartup = m_Session.ProjectData.StartupScene;
        m_SettingsInitialized = true;
    }
    ImGui::SetNextWindowSize({960,640},ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Preferences / Project Settings",&m_ShowProjectSettings)) { ImGui::End(); return; }
    ImGui::SetNextItemWidth(-1);
    EditText("##Search Settings",m_SettingsSearch);
    if (!ImGui::IsItemActive() && m_SettingsSearch.empty())
    {
        const auto origin = ImGui::GetItemRectMin();
        const auto padding = ImGui::GetStyle().FramePadding;
        ImGui::GetWindowDrawList()->AddText({origin.x + padding.x,origin.y + padding.y},ImGui::GetColorU32(ImGuiCol_TextDisabled),"Search settings");
    }
    ImGui::Separator();
    const char* categories[] = {"Project","Editor","Assets","Native Scripts","Rendering","External Tools","Keymaps","Player"};
    const char* keywords[] = {
        "Project Name | Startup Scene | Location",
        "Theme | TOML | Appearance | Camera Move Speed | Camera Sprint Multiplier | Camera Look Sensitivity | Save Window Layout",
        "Model Scale | Generate Missing Normals | Flip Model UVs | Assign Imported Materials | Thumbnail Render Resolution | Rendered Thumbnail Cache | Automatically Reimport Changed Assets | Asset Scan Interval",
        "Build Project Scripts | Reload Compiled Module | Automatically Build and Reload Scripts | Script Build Delay | Build Diagnostics | Source Directory",
        "Show Scene Grid | Grid Spacing | Grid Fade Distance | Scene Background | Vulkan",
        "External Tools | Visual Studio | Visual Studio Code | Script Editor | Regenerate Solution",
        "Keymaps | Delete | Duplicate | Rename | Keyboard | Preferences",
        "Player | Company Name | Product Name | Version | Resolution | Fullscreen"
    };
    const auto matches = [&](int index) { return m_SettingsSearch.empty() || MatchesSearch(std::string(categories[index]) + " " + keywords[index],m_SettingsSearch); };
    if (!matches(m_SettingsCategory))
        for (int index = 0; index < 8; ++index)
            if (matches(index)) { m_SettingsCategory = index; break; }
    ImGui::BeginChild("Settings Categories",{190,-45},ImGuiChildFlags_Borders);
    for (int i = 0; i < 8; ++i)
    {
        if (!matches(i)) continue;
        if (ImGui::Selectable(categories[i],m_SettingsCategory == i)) m_SettingsCategory = i;
    }
    ImGui::EndChild(); ImGui::SameLine();
    ImGui::BeginChild("Settings Properties",{0,-45},ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(categories[m_SettingsCategory]);
    ImGui::Separator();
    auto show = [&](const char* label) { return m_SettingsSearch.empty() || MatchesSearch(label,m_SettingsSearch) || MatchesSearch(categories[m_SettingsCategory],m_SettingsSearch); };
    auto field = [](const char* label)
    {
        const float width = ImGui::GetContentRegionAvail().x;
        const float start = ImGui::GetCursorPosX();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        if (width > 410) ImGui::SameLine(start + std::max(width * 0.4f,260.0f));
        ImGui::SetNextItemWidth(-1);
        return std::string("##") + label;
    };
    auto number = [&](const char* label, float& value, float step, float minimum, float maximum)
    { if (show(label)) ImGui::DragFloat(field(label).c_str(),&value,step,minimum,maximum,"%.3f",ImGuiSliderFlags_AlwaysClamp); };
    auto toggle = [&](const char* label, bool& value) { if (show(label)) ImGui::Checkbox(field(label).c_str(),&value); };
    auto& draft = m_SettingsDraft;
    if(m_SettingsCategory == 7) m_BuildPanel.PlayerProperties(m_Session.ProjectData);
    else if (m_SettingsCategory == 0)
    {
        if (ImGui::Button("Choose Project Thumbnail..."))
        {
            const auto image = FileDialogs::OpenFile("Images\0*.png;*.jpg;*.jpeg\0\0");
            if (!image.empty())
            {
                const auto target = m_Session.ProjectData.GetDirectory() / "Settings/ProjectThumbnail.png";
                std::error_code error;
                if (!std::filesystem::equivalent(image,target,error))
                    std::filesystem::copy_file(image, target,std::filesystem::copy_options::overwrite_existing,error);
                if (error) m_ThemeMessage = error.message();
                AssetManager::InvalidateAssetPath(target.string());
                AssetPreview::Clear();
            }
        }
        if (show("Project Name")) EditText(field("Project Name").c_str(),m_SettingsName);
        if (show("Startup Scene"))
        {
            ImGui::TextWrapped("Startup Scene: %s",m_SettingsStartup.c_str());
            if (ImGui::Button("Select Startup Scene"))
                m_AssetPicker.Open(ProjectAssetType::Scene,AssetRegistry::Reference(m_SettingsStartup),[this](const std::string& reference)
                { m_SettingsStartup = reference.empty() ? "" : std::filesystem::path(AssetRegistry::Resolve(reference)).lexically_relative(m_Session.ProjectData.GetDirectory()).generic_string(); });
        }
        ImGui::TextWrapped("Project: %s",m_Session.ProjectData.GetPath().string().c_str());
        ImGui::TextWrapped("Assets: %s",m_Session.ProjectData.GetAssetsDirectory().string().c_str());
    }
    else if (m_SettingsCategory == 1)
    {
        if (show("Theme"))
        {
            const auto choices = EditorTheme::List(m_EngineAssets, m_Session.ProjectData.GetDirectory());
            const auto currentTheme = std::find_if(choices.begin(), choices.end(), [&](const auto& choice) { return choice.ID == draft.Theme; });
            const auto& themeLabel = currentTheme == choices.end() ? draft.Theme : currentTheme->Name;
            if (ImGui::BeginCombo(field("Theme").c_str(), themeLabel.c_str()))
            {
                for (const auto& choice : choices)
                    if (ImGui::Selectable(choice.Name.c_str(), draft.Theme == choice.ID)) draft.Theme = choice.ID;
                ImGui::EndCombo();
            }
            if (ImGui::Button("Reload Selected Theme"))
            {
                ImGuiStyle candidate;
                if (EditorTheme::Load(draft.Theme, m_EngineAssets, m_Session.ProjectData.GetDirectory(), candidate, m_ThemeMessage))
                {
                    m_Session.ProjectData.Settings.Theme = draft.Theme;
                    m_ThemePending = true;
                    m_Session.ProjectData.SaveSettings();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Export Theme as TOML"))
            {
                const auto destination = m_Session.ProjectData.GetDirectory() / "Settings/Themes/Custom.toml";
                if (EditorTheme::Export(destination, "Custom", m_ThemeMessage))
                    m_ThemeMessage = "Saved Settings/Themes/Custom.toml";
            }
            ImGui::TextWrapped("Custom themes load from Settings/Themes. Select a theme and apply to update every editor panel.");
            if (!m_ThemeMessage.empty()) ImGui::TextWrapped("%s", m_ThemeMessage.c_str());
        }
        number("Camera Move Speed",draft.CameraSpeed,0.1f,0.01f,1000);
        number("Camera Sprint Multiplier",draft.CameraSprint,0.1f,1,20);
        number("Camera Look Sensitivity",draft.CameraSensitivity,0.01f,0.001f,5);
        if (ImGui::Button("Save Window Layout")) ImGui::SaveIniSettingsToDisk(m_LayoutPath.c_str());
    }
    else if (m_SettingsCategory == 2)
    {
        if (ImGui::CollapsingHeader("Model Import",ImGuiTreeNodeFlags_DefaultOpen))
        {
            number("Model Scale",draft.Models.Scale,0.01f,0.0001f,10000);
            toggle("Convert FBX Units To Metres",draft.Models.ConvertUnits);
            toggle("Generate Missing Normals",draft.Models.GenerateNormals);
            toggle("Flip Model UVs",draft.Models.FlipUVs);
            toggle("Assign Imported Materials",draft.Models.ImportMaterials);
            ImGui::TextWrapped("Import options apply to reimported and newly instantiated models. Existing scene transforms remain editable.");
        }
        if (ImGui::CollapsingHeader("Thumbnails",ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (show("Thumbnail Render Resolution")) ImGui::SliderInt(field("Thumbnail Render Resolution").c_str(),&draft.ThumbnailSize,64,512);
            if (show("Rendered Thumbnail Cache")) ImGui::SliderInt(field("Rendered Thumbnail Cache").c_str(),&draft.ThumbnailCapacity,16,512);
            if (ImGui::Button("Rebuild Thumbnail Cache")) AssetPreview::Clear();
        }
        toggle("Automatically Reimport Changed Assets",draft.AutoReimport);
        number("Asset Scan Interval",draft.AssetScanInterval,0.1f,0.5f,60);
    }
    else if (m_SettingsCategory == 3)
    {
        ImGui::TextWrapped("%s",m_Session.Scripts.GetStatus().c_str());
        ImGui::Text("Loaded generation: %llu",static_cast<unsigned long long>(m_Session.Scripts.GetGeneration()));
        ImGui::TextWrapped("Source directory: %s",draft.ScriptDirectory.c_str());
        toggle("Automatically Build and Reload Scripts",draft.AutoBuildScripts);
        number("Script Build Delay",draft.ScriptBuildDelay,0.05f,0.1f,10);
        ImGui::BeginDisabled(m_Session.Scripts.IsBuilding());
        if (ImGui::Button("Build Project Scripts")) m_Session.Scripts.Build();
        ImGui::SameLine(); if (ImGui::Button("Reload Compiled Module")) m_Session.Scripts.Reload();
        ImGui::EndDisabled();
        ImGui::TextWrapped("Successful builds replace script types in Edit mode and preserve registered fields. Builds completed during Play wait until Stop.");
        if (ImGui::CollapsingHeader("Build Diagnostics"))
        {
            ImGui::BeginChild("Build Output",{0,220},ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar);
            const auto& diagnostics = m_Session.Scripts.GetDiagnostics();
            ImGui::TextUnformatted(diagnostics.data(),diagnostics.data() + diagnostics.size());
            ImGui::EndChild();
        }
    }
    else if (m_SettingsCategory == 6)
    {
        DrawShortcutSettings();
    }
    else if (m_SettingsCategory == 5)
    {
        ImGui::TextWrapped("Script Editor: %s", m_Session.Tools.Executable.c_str());
        if (ImGui::BeginCombo("Detected Script Editors", m_Session.Tools.Executable.c_str()))
        {
            for (const auto& executable : m_Session.Tools.Installations)
                if (ImGui::Selectable(executable.c_str(), executable == m_Session.Tools.Executable))
                { m_Session.Tools.Executable = executable; m_Session.Tools.Save(); }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Browse Editor..."))
        {
            const auto executable = FileDialogs::OpenFile("Applications\0*.exe\0All Files\0*.*\0\0");
            if (!executable.empty()) { m_Session.Tools.Executable = executable; m_Session.Tools.Save(); }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Detect")) m_Session.Tools.Detect();
        static std::future<ProcessResult> generation;
        static std::string diagnostics;
        if (generation.valid() && generation.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        { const auto result = generation.get(); diagnostics = result.ExitCode == 0 ? "Solution regenerated" : result.Output; }
        ImGui::BeginDisabled(generation.valid() || m_Session.Scripts.IsBuilding());
        if (ImGui::SmallButton("Regenerate Solution"))
        {
            auto engine = m_EngineAssets;
            while (!engine.empty() && !std::filesystem::exists(engine / "Scripts/Templates/ProjectScripts.lua"))
            { const auto parent = engine.parent_path(); if (parent == engine) { engine.clear(); break; } engine = parent; }
            const auto project = m_Session.ProjectData.GetDirectory();
            const auto bin = Process::ExecutablePath().parent_path();
            const auto scripts = m_Session.ProjectData.Settings.ScriptDirectory;
            const auto projectName=m_Session.ProjectData.GetName();
            generation = std::async(std::launch::async, [engine,project,bin,scripts,projectName]
            {
#ifdef NDEBUG
                const std::wstring configuration = L"Release";
#else
                const std::wstring configuration = L"Debug";
#endif
                return Process::Run({(bin / "FIBuild.exe").wstring(),L"native",L"--project",project.wstring(),L"--name",std::filesystem::path(projectName).wstring(),L"--engine",engine.wstring(),
                    L"--bin",bin.wstring(),L"--source",std::filesystem::path(scripts).wstring(),L"--configuration",configuration,L"--generate-only"},project);
            });
        }
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", m_Session.Tools.Status.c_str());
        ImGui::TextWrapped("%s", diagnostics.c_str());
    }
    else
    {
        ImGui::TextUnformatted("Renderer: Vulkan");
        toggle("Show Scene Grid",draft.Grid);
        number("Grid Spacing",draft.GridSpacing,0.1f,0.01f,1000);
        number("Grid Fade Distance",draft.GridFadeDistance,1,10,10000);
        if (show("Scene Background")) ImGui::ColorEdit4(field("Scene Background").c_str(),&draft.SceneBackground.x);
        ImGui::TextWrapped("The grid is editor-only and does not appear in the Game view.");
    }
    ImGui::EndChild();
    ImGui::Separator();
    if (ImGui::Button("Apply and Save"))
    {
        if (!m_SettingsName.empty())
        {
            ImGuiStyle candidate;
            if (EditorTheme::Load(draft.Theme, m_EngineAssets, m_Session.ProjectData.GetDirectory(), candidate, m_ThemeMessage))
            {
                m_Session.ProjectData.Settings = draft; m_Session.ProjectData.SetName(m_SettingsName); m_Session.Tools.ProjectName=ExternalTools::SafeName(m_SettingsName); m_Session.ProjectData.StartupScene = m_SettingsStartup;
                if (m_Session.ProjectData.Save()) { ApplyProjectSettings(); AssetPreview::Clear(); AssetRegistry::Scan(); }
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Revert")) { m_SettingsDraft = m_Session.ProjectData.Settings; m_SettingsName = m_Session.ProjectData.GetName(); m_SettingsStartup = m_Session.ProjectData.StartupScene; }
    ImGui::End();
}
