#include "Project.h"
#include <Core/Log.h>
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>

bool FINALITY::Project::LoadSettings()
{
    try
    {
        Settings = ProjectSettings();
        const auto path = GetDirectory() / "Settings/Project.yaml";
        if (!std::filesystem::exists(path)) return true;
        const auto data = YAML::LoadFile(path.string());
        if (data["Version"].as<int>(1) != 1) throw std::runtime_error("Unsupported settings version");
        Settings.Theme = data["Theme"].as<std::string>("Dark01");
        Settings.CameraSpeed = std::clamp(data["CameraSpeed"].as<float>(4), 0.01f,1000.0f);
        Settings.CameraSprint = std::clamp(data["CameraSprint"].as<float>(3), 1.0f,20.0f);
        Settings.CameraSensitivity = std::clamp(data["CameraSensitivity"].as<float>(0.12f), 0.001f,5.0f);
        if (const auto color = data["SceneBackground"]; color && color.size() == 4)
            for (int i = 0; i < 4; ++i) Settings.SceneBackground[i] = std::clamp(color[i].as<float>(),0.0f,1.0f);
        Settings.Grid = data["Grid"].as<bool>(true);
        Settings.GridSpacing = std::clamp(data["GridSpacing"].as<float>(1),0.01f,1000.0f);
        Settings.GridFadeDistance = std::clamp(data["GridFadeDistance"].as<float>(180),10.0f,10000.0f);
        Settings.ThumbnailSize = std::clamp(data["ThumbnailSize"].as<int>(128),64,512);
        Settings.ThumbnailCapacity = std::clamp(data["ThumbnailCapacity"].as<int>(128),16,512);
        Settings.AutoReimport = data["AutoReimport"].as<bool>(true);
        Settings.AssetScanInterval = std::clamp(data["AssetScanInterval"].as<float>(2),0.5f,60.0f);
        Settings.AutoBuildScripts = data["AutoBuildScripts"].as<bool>(true);
        Settings.ScriptBuildDelay = std::clamp(data["ScriptBuildDelay"].as<float>(0.75f),0.1f,10.0f);
        Settings.ScriptDirectory = data["ScriptDirectory"].as<std::string>("Assets/Scripts");
        const std::filesystem::path directory(Settings.ScriptDirectory);
        if (directory.has_root_path() || directory.empty() || directory.lexically_normal() == "." || *directory.lexically_normal().begin() == "..")
            throw std::runtime_error("Script directory must be project-relative");
        if (const auto models = data["Models"])
        {
            Settings.Models.Scale = std::clamp(models["Scale"].as<float>(1),0.0001f,10000.0f);
            Settings.Models.GenerateNormals = models["GenerateNormals"].as<bool>(true);
            Settings.Models.FlipUVs = models["FlipUVs"].as<bool>(false);
            Settings.Models.ImportMaterials = models["ImportMaterials"].as<bool>(true);
        }
        return true;
    }
    catch (const std::exception& error) { Settings = ProjectSettings(); FI_CORE_ERROR("Project settings: {}", error.what()); return false; }
}

bool FINALITY::Project::SaveSettings() const
{
    if (!IsOpen()) return false;
    try
    {
        std::filesystem::create_directories(GetDirectory() / "Settings");
        YAML::Emitter out;
        out << YAML::BeginMap << YAML::Key << "Version" << YAML::Value << 1;
        auto value = [&](const char* name, const auto& item) { out << YAML::Key << name << YAML::Value << item; };
        value("Theme", Settings.Theme);
        value("CameraSpeed", Settings.CameraSpeed); value("CameraSprint", Settings.CameraSprint); value("CameraSensitivity", Settings.CameraSensitivity);
        out << YAML::Key << "SceneBackground" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (int i = 0; i < 4; ++i) out << Settings.SceneBackground[i];
        out << YAML::EndSeq;
        value("Grid", Settings.Grid); value("GridSpacing", Settings.GridSpacing); value("GridFadeDistance", Settings.GridFadeDistance);
        value("ThumbnailSize", Settings.ThumbnailSize); value("ThumbnailCapacity", Settings.ThumbnailCapacity);
        value("AutoReimport", Settings.AutoReimport); value("AssetScanInterval", Settings.AssetScanInterval);
        value("AutoBuildScripts", Settings.AutoBuildScripts); value("ScriptBuildDelay", Settings.ScriptBuildDelay); value("ScriptDirectory", Settings.ScriptDirectory);
        out << YAML::Key << "Models" << YAML::Value << YAML::BeginMap;
        value("Scale",Settings.Models.Scale); value("GenerateNormals",Settings.Models.GenerateNormals); value("FlipUVs",Settings.Models.FlipUVs); value("ImportMaterials",Settings.Models.ImportMaterials);
        out << YAML::EndMap << YAML::EndMap;
        const auto target = GetDirectory() / "Settings/Project.yaml";
        const auto temporary = target.string() + ".tmp";
        std::ofstream stream(temporary);
        stream << out.c_str(); stream.close();
        if (!stream) return false;
        std::filesystem::copy_file(temporary, target, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(temporary);
        return true;
    }
    catch (const std::exception& error) { FI_CORE_ERROR("Cannot save project settings: {}", error.what()); return false; }
}
