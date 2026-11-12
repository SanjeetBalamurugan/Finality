#include "Project.h"
#include <Core/Log.h>
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <Utils/Serializers/SceneSerializer.h>

bool FINALITY::Project::Create(const std::filesystem::path& location, const std::string& name,
    const std::filesystem::path& templateAssets, bool starterTemplate)
{
    namespace fs = std::filesystem;
    try
    {
        if (name.empty() || name == "." || name == ".." || name.find_first_of("<>:\"/\\|?*") != std::string::npos)
            throw std::runtime_error("Enter a valid project name");
        const fs::path root = fs::absolute(location / name).lexically_normal();
        if (fs::exists(root) && !fs::is_empty(root))
            throw std::runtime_error("Choose an empty project directory");
        for (const auto* directory : {"Assets/Scenes", "Assets/Scripts", "Assets/Materials", "Assets/Shaders", "Assets/Textures", "Assets/Models", "Settings", ".Finality"})
            fs::create_directories(root / directory);
        if (!templateAssets.empty() && fs::is_directory(templateAssets))
        {
            if (starterTemplate)
            {
                for (const auto* directory : {"shaders", "textures", "materials"})
                    if (fs::is_directory(templateAssets / directory)) fs::copy(templateAssets / directory, root / "Assets" / directory, fs::copy_options::recursive | fs::copy_options::skip_existing);
            }
            else
                for (const auto* resource : {"shaders/shader.vert", "shaders/shader.frag", "textures/test.png", "materials/Default.fmat"})
                {
                    fs::create_directories((root / "Assets" / resource).parent_path());
                    fs::copy_file(templateAssets / resource, root / "Assets" / resource, fs::copy_options::skip_existing);
                }
        }
        Project created;
        created.m_Path = root / (name + ".fproj");
        created.m_Name = name;
        EmptyScene scene;
        auto camera = scene.CreateEntity("Main Camera");
        camera.AddComponent<CameraComponent>(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
        camera.GetComponent<TransformComponent>().Position = {0,1.5f,5};
        camera.GetComponent<TransformComponent>().Rotation = {-15,0,0};
        SceneSerializer(scene, "Main").Serialize((root / created.StartupScene).string());
        if (!created.Save()) return false;
        *this = std::move(created);
        return true;
    }
    catch (const std::exception& error)
    {
        FI_CORE_ERROR("Cannot create project: {}", error.what());
        return false;
    }
}

bool FINALITY::Project::Open(const std::filesystem::path& path)
{
    try
    {
        const auto data = YAML::LoadFile(path.string())["Project"];
        if (!data || data["Version"].as<int>(0) != 1)
            throw std::runtime_error("Unsupported project format");
        const std::string startup = data["StartupScene"].as<std::string>("");
        const std::filesystem::path relative(startup);
        if (relative.is_absolute() || (!relative.empty() && *relative.lexically_normal().begin() == ".."))
            throw std::runtime_error("Startup scene must be inside the project");
        m_Path = std::filesystem::absolute(path).lexically_normal();
        m_Name = data["Name"].as<std::string>();
        StartupScene = startup;
        LoadSettings();
        std::filesystem::create_directories(GetAssetsDirectory());
        return true;
    }
    catch (const std::exception& error)
    {
        FI_CORE_ERROR("Cannot open project '{}': {}", path.string(), error.what());
        return false;
    }
}

bool FINALITY::Project::Save() const
{
    if (!IsOpen()) return false;
    YAML::Emitter out;
    out << YAML::BeginMap << YAML::Key << "Project" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Version" << YAML::Value << 1;
    out << YAML::Key << "Name" << YAML::Value << m_Name;
    out << YAML::Key << "StartupScene" << YAML::Value << StartupScene;
    out << YAML::EndMap << YAML::EndMap;
    std::ofstream stream(m_Path);
    stream << out.c_str();
    return stream.good() && SaveSettings();
}
