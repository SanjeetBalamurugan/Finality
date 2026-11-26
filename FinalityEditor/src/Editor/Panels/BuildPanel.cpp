#include "BuildPanel.h"
#include "../DevelopmentSDK.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <imgui.h>
#include <shellapi.h>
#define YAML_CPP_STATIC_DEFINE
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace
{
    bool Text(const char* label, std::string& value)
    {
        std::array<char, 1024> buffer{};
        std::copy_n(value.data(), std::min(value.size(), buffer.size() - 1), buffer.data());
        if (!ImGui::InputText(label, buffer.data(), buffer.size()))
        {
            return false;
        }
        value = buffer.data();
        return true;
    }
} // namespace

void FINALITY::BuildPanel::Load(const Project& project)
{
    if (Root == project.GetDirectory())
    {
        return;
    }
    Root = project.GetDirectory();
    Scenes.clear();
    Product = project.GetName();
    Company = "DefaultCompany";
    DefaultIcon.clear();
    Icons.clear();
    Version = "0.1.0";
    Width = 1280;
    Height = 720;
    Fullscreen = CopyPDB = false;
    Output.clear();
    try
    {
        auto path = Root / "Settings/Build.yaml";
        if (std::filesystem::exists(path))
        {
            auto data = YAML::LoadFile(path.string());
            Product = data["ProductName"].as<std::string>(Product);
            Company = data["CompanyName"].as<std::string>(Company);
            Version = data["ProductVersion"].as<std::string>(Version);
            DefaultIcon = data["DefaultIcon"].as<std::string>("");
            if (const auto icons = data["Icons"])
            {
                constexpr int iconSizes[] =
                {
                    16,
                    24,
                    32,
                    48,
                    64,
                    128,
                    256
                };

                for (const int size : iconSizes)
                {
                    Icons[size] = icons[std::to_string(size)].as<std::string>("");
                }
            }
            Width = std::clamp(data["Width"].as<int>(1280), 320, 16384);
            Height = std::clamp(data["Height"].as<int>(720), 200, 16384);
            Fullscreen = data["Fullscreen"].as<bool>(false);
            CopyPDB = data["CopyPDB"].as<bool>(false);
            if (auto list = data["Scenes"])
            {
                for (auto node : list)
                {
                    Scenes.push_back({ node["Path"].as<std::string>(), node["Enabled"].as<bool>(true) });
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        Status = e.what();
    }
    if (Scenes.empty() && !project.StartupScene.empty())
    {
        Scenes.push_back({ project.StartupScene, true });
    }
}

void FINALITY::BuildPanel::Save()
{
    std::filesystem::create_directories(Root / "Settings");
    YAML::Emitter out;
    out << YAML::BeginMap << YAML::Key << "ProductName" << YAML::Value << Product << YAML::Key
        << "CompanyName" << YAML::Value << Company << YAML::Key << "ProductVersion" << YAML::Value << Version
        << YAML::Key << "Width" << YAML::Value << Width << YAML::Key << "Height" << YAML::Value << Height
        << YAML::Key << "Fullscreen" << YAML::Value << Fullscreen << YAML::Key << "CopyPDB" << YAML::Value
        << CopyPDB << YAML::Key << "Scenes" << YAML::Value << YAML::BeginSeq;
    for (auto& scene : Scenes)
    {
        out << YAML::BeginMap << YAML::Key << "Path" << YAML::Value << scene.Path << YAML::Key << "Enabled"
            << YAML::Value << scene.Enabled << YAML::EndMap;
    }
    out << YAML::EndSeq << YAML::Key << "DefaultIcon" << YAML::Value << DefaultIcon << YAML::Key << "Icons" << YAML::Value << YAML::BeginMap;
    for (const auto& [size, path] : Icons)
    {
        out << YAML::Key << std::to_string(size) << YAML::Value << path;
    }
    out << YAML::EndMap << YAML::EndMap;
    std::ofstream(Root / "Settings/Build.yaml") << out.c_str();
}

void FINALITY::BuildPanel::PlayerProperties(const Project& project)
{
    Load(project);
    bool changed = Text("Company Name", Company);
    changed |= Text("Product Name", Product);
    changed |= Text("Version", Version);
    ImGui::TextDisabled("Company, product and version are written to executable properties.");
    ImGui::SeparatorText("Icons and Dimensions");
    ImGui::TextWrapped("Leave images empty to use Finality's icon. A default square image fills missing sizes; overrides must match the listed dimensions. The multi-resolution ICO is generated when building.");
    const auto iconField = [&](const std::string& label, std::string& path)
    {
        ImGui::PushID(label.c_str());
        bool edited = Text(label.c_str(), path);
        ImGui::SameLine();
        if (ImGui::SmallButton("Browse"))
        {
            const auto selected = FileDialogs::OpenFile("Images\0*.png;*.ico;*.jpg;*.jpeg;*.bmp\0");
            if (!selected.empty())
            {
                const auto file = std::filesystem::weakly_canonical(std::filesystem::u8path(selected));
                auto relative = file.lexically_relative(std::filesystem::weakly_canonical(Root / "Assets"));
                if (!relative.empty() && *relative.begin() != "..")
                {
                    path = file.lexically_relative(Root).generic_string();
                }
                else if (AssetRegistry::Import(file, "Assets/Icons"))
                {
                    path = (std::filesystem::path("Assets/Icons") / file.filename()).generic_string();
                }
                edited = true;
            }
        }
        ImGui::PopID();
        return edited;
    };
    changed |= iconField("Default Icon", DefaultIcon);
    constexpr int displayIconSizes[] =
    {
        16,
        24,
        32,
        48,
        64,
        128,
        256
    };

    for (const int size : displayIconSizes)
    {
        changed |= iconField(std::to_string(size) + " x " + std::to_string(size), Icons[size]);
    }
    ImGui::SeparatorText("Resolution and Presentation");
    changed |= ImGui::InputInt("Default Width", &Width);
    changed |= ImGui::InputInt("Default Height", &Height);
    changed |= ImGui::Checkbox("Fullscreen", &Fullscreen);
    ImGui::TextDisabled("Windows x64 | Vulkan | Optimized player");
    Width = std::clamp(Width, 320, 16384);
    Height = std::clamp(Height, 200, 16384);
    if (changed)
    {
        Save();
    }
}

void FINALITY::BuildPanel::Start(Project& project, const std::filesystem::path& engineAssets, bool run)
{
    if (Job.valid())
    {
        return;
    }
    Load(project);
    try
    {
        auto selected = FileDialogs::OpenFolder();
        if (selected.empty())
        {
            return;
        }
        Output = std::filesystem::absolute(selected);
        auto bin = Process::ExecutablePath().parent_path();
#ifndef NDEBUG
        const auto parent = bin.parent_path();
        if (std::filesystem::is_regular_file(parent / "Dist-windows-x86_64/FIBaker.exe"))
        {
            bin = parent / "Dist-windows-x86_64";
        }
        else if (std::filesystem::is_regular_file(parent / "Release-windows-x86_64/FIBaker.exe"))
        {
            bin = parent / "Release-windows-x86_64";
        }
#endif
        auto engine = FindDevelopmentRoot(bin, engineAssets);
        if (engine.empty())
        {
            throw std::runtime_error("Player build tools are missing");
        }
        AssetRegistry::Scan();
        project.Save();
        Save();
        YAML::Emitter request;
        request.SetMapFormat(YAML::Flow);
        request.SetSeqFormat(YAML::Flow);
        request.SetStringFormat(YAML::DoubleQuoted);
        request << YAML::BeginMap;
        auto value = [&](const char* key, const auto& v)
        {
            request << YAML::Key << key << YAML::Value << v;
        };
        value("ProjectRoot", Root.string());
        value("ProjectName", project.GetName());
        value("Output", Output.string());
        value("Platform", "Windows");
        value("ProductName", Product);
        value("CompanyName", Company);
        value("Version", Version);
        value("DefaultIcon", DefaultIcon);
        request << YAML::Key << "Icons" << YAML::Value << YAML::BeginMap;
        for (const auto& [size, icon] : Icons)
        {
            request << YAML::Key << std::to_string(size) << YAML::Value << icon;
        }
        request << YAML::EndMap;
        value("Width", Width);
        value("Height", Height);
        value("Fullscreen", Fullscreen);
        value("CopyPDB", CopyPDB);
        value("ScriptDirectory", project.Settings.ScriptDirectory);
        request << YAML::Key << "Scenes" << YAML::Value << YAML::BeginSeq;
        int enabled = 0;
        for (auto& scene : Scenes)
        {
            if (scene.Enabled)
            {
                request << scene.Path;
                ++enabled;
            }
        }
        request << YAML::EndSeq << YAML::EndMap;
        if (!enabled)
        {
            throw std::runtime_error("Enable at least one scene");
        }
        std::filesystem::create_directories(Root / ".Finality");
        const auto path = Root / ".Finality/BuildRequest.json";
        std::ofstream(path) << request.c_str();
        if (!std::filesystem::is_regular_file(bin / "FIBaker.exe") || !std::filesystem::is_regular_file(bin / "FinalityPlayer.exe"))
        {
            throw std::runtime_error("Player build tools are missing. Build FIBaker and FinalityPlayer first.");
        }
        Launch = Output / "Game.exe";
        Run = run;
        Job = std::async(std::launch::async, [engine, bin, path]()
                         {
                             return Process::Run({ (bin / "FIBaker.exe").wstring(), L"--request", path.wstring(), L"--engine",
                                                   engine.wstring(), L"--bin", bin.wstring() },
                                                 engine);
                         });
        Status = "Compiling scripts and packaging player...";
    }
    catch (const std::exception& e)
    {
        Status = e.what();
        Visible = true;
    }
}

void FINALITY::BuildPanel::Draw(Project& project, const std::filesystem::path& assets,
                                const std::string& current, bool canBuild)
{
    if (Job.valid() && Job.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        auto result = Job.get();
        Status = result.ExitCode == 0 ? "Build succeeded: " + Launch.parent_path().string() : result.Output;
        if (result.ExitCode == 0)
        {
            const auto folder = Launch.parent_path();
            if (reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
            {
                Status += " (could not open output folder)";
            }
        }
        if (result.ExitCode == 0 && Run)
        {
            auto code = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", Launch.c_str(), nullptr, Launch.parent_path().c_str(), SW_SHOWNORMAL));
            if (code <= 32)
            {
                Status = "Build succeeded but the player could not launch";
            }
        }
        Visible = true;
    }
    if (!project.IsOpen())
    {
        return;
    }
    Load(project);
    auto& io = ImGui::GetIO();
    if (canBuild && !io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B))
    {
        if (io.KeyShift)
        {
            Visible = true;
        }
        else
        {
            Start(project, assets, true);
        }
    }
    if (!Visible)
    {
        return;
    }
    if (!ImGui::Begin("Build Settings", &Visible))
    {
        ImGui::End();
        return;
    }
    ImGui::BeginDisabled(Job.valid());
    ImGui::SeparatorText("Scenes In Build");
    int index = 0, remove = -1, moveFrom = -1, moveTo = -1;
    bool changed = false;
    for (size_t i = 0; i < Scenes.size(); ++i)
    {
        ImGui::PushID(int(i));
        changed |= ImGui::Checkbox("##Enabled", &Scenes[i].Enabled);
        ImGui::SameLine();
        const auto label = Scenes[i].Path + (Scenes[i].Enabled ? "    " + std::to_string(index++) : "");
        ImGui::Selectable(label.c_str(), false);
        if (ImGui::BeginDragDropSource())
        {
            const int source = static_cast<int>(i);
            ImGui::SetDragDropPayload("FINALITY_BUILD_SCENE", &source, sizeof(source));
            ImGui::TextUnformatted(Scenes[i].Path.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const auto* payload = ImGui::AcceptDragDropPayload("FINALITY_BUILD_SCENE"))
            {
                if (payload->DataSize == sizeof(int))
                {
                    moveFrom = *static_cast<const int*>(payload->Data);
                    moveTo = static_cast<int>(i);
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Move Up", nullptr, false, i > 0))
            {
                std::swap(Scenes[i], Scenes[i - 1]);
                changed = true;
            }
            if (ImGui::MenuItem("Move Down", nullptr, false, i + 1 < Scenes.size()))
            {
                std::swap(Scenes[i], Scenes[i + 1]);
                changed = true;
            }
            if (ImGui::MenuItem("Remove"))
            {
                remove = int(i);
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    if (moveFrom >= 0 && moveFrom < static_cast<int>(Scenes.size()) && moveTo >= 0 && moveTo < static_cast<int>(Scenes.size()) && moveFrom != moveTo)
    {
        auto scene = std::move(Scenes[moveFrom]);
        Scenes.erase(Scenes.begin() + moveFrom);
        Scenes.insert(Scenes.begin() + moveTo, std::move(scene));
        changed = true;
    }
    if (remove >= 0)
    {
        Scenes.erase(Scenes.begin() + remove);
        changed = true;
    }
    if (ImGui::Button("Add Open Scene") && !current.empty())
    {
        auto path = std::filesystem::path(current);
        if (path.is_absolute())
        {
            path = path.lexically_relative(Root);
        }
        if (!path.empty() && *path.begin() != ".." &&
            std::none_of(Scenes.begin(), Scenes.end(),
                         [&](const auto& s)
                         {
                             return s.Path == path.generic_string();
                         }))
        {
            Scenes.push_back({ path.generic_string(), true });
            changed = true;
        }
    }
    if (ImGui::BeginCombo("Add Scene Asset", "Choose scene..."))
    {
        for (auto& [id, record] : AssetRegistry::GetRecords())
        {
            if (record.Type == ProjectAssetType::Scene && ImGui::Selectable(record.Path.c_str()))
            {
                if (std::none_of(Scenes.begin(), Scenes.end(),
                                 [&](const auto& s)
                                 {
                                     return s.Path == record.Path;
                                 }))
                {
                    Scenes.push_back({ record.Path, true });
                    changed = true;
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SeparatorText("Platform");
    ImGui::TextUnformatted("Windows | x86-64 | Vulkan");
    ImGui::BeginDisabled();
    ImGui::Button("Linux (toolchain not available yet)");
    ImGui::EndDisabled();
    changed |= ImGui::Checkbox("Copy PDB Files", &CopyPDB);
    if (ImGui::Button("Player Settings..."))
    {
        OpenPlayerSettings = true;
    }
    if (changed)
    {
        Save();
    }
    ImGui::TextWrapped("The first enabled scene starts the game. Drag scenes to reorder; right-click to remove. "
                       "Save scene changes before building. Choose a new or empty output folder.");
    ImGui::BeginDisabled(!canBuild);
    if (ImGui::Button("Build"))
    {
        Start(project, assets, false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Build And Run"))
    {
        Start(project, assets, true);
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::Separator();
    ImGui::TextWrapped("%s", Status.c_str());
    ImGui::End();
}
