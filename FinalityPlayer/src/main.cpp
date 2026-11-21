#include <Core/Process.h>
#include <Finality.h>
#define YAML_CPP_STATIC_DEFINE
#include <cstdlib>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
namespace fs = std::filesystem;
using namespace FINALITY;
class Player : public Game
{
    fs::path root, cache;
    YAML::Node manifest;
    ScriptModule module;
    int frames = 0, limit = 0;

  public:
    Player(fs::path directory, YAML::Node data, int frameLimit)
        : root(std::move(directory)), manifest(data), limit(frameLimit)
    {
    }
    void Init() override
    {
        const auto *local = std::getenv("LOCALAPPDATA");
        cache = (local ? fs::path(local) : fs::temp_directory_path()) / "Finality/Players" /
                std::to_string(std::hash<std::string>{}(root.string()));
        fs::create_directories(cache);
        ImGui::GetIO().IniFilename = nullptr;
        AssetRegistry::SetRuntimeCacheDirectory(cache);
        ModelImportOptions options;
        if (auto models = manifest["Models"])
        {
            options.Scale = models["Scale"].as<float>(1);
            options.ConvertUnits = models["ConvertUnits"].as<bool>(true);
            options.GenerateNormals = models["GenerateNormals"].as<bool>(true);
            options.FlipUVs = models["FlipUVs"].as<bool>(false);
            options.ImportMaterials = models["ImportMaterials"].as<bool>(true);
        }
        ModelAsset::Configure(root / "Data/Builtin",options);
        AssetRegistry::Open(root / "Data");
        const auto scripts = root / "GameScripts.dll";
        if (fs::is_regular_file(scripts))
        {
            std::string error;
            if (!module.Load(scripts, cache / "Modules", nullptr, error))
                throw std::runtime_error(error);
        }
        auto scene = std::make_unique<EmptyScene>();
        const auto first = manifest["Scenes"][0].as<std::string>();
        if (!SceneSerializer(*scene).DeSerialize((root / "Data" / first).string()))
            throw std::runtime_error("Cannot load startup scene: " + first);
        SceneManager::GetInstance().SetScene(std::move(scene));
        SceneManager::GetInstance().ChangeScene();
    }
    void Update(float) override
    {
        if (limit > 0 && ++frames >= limit)
            Application::Get().RequestClose();
    }
    void Destroy() override
    {
    }
};
int main(int argc, char **argv)
{
    try
    {
        const auto root = Process::ExecutablePath().parent_path();
        fs::current_path(root);
        const auto data = YAML::LoadFile((root / "Player.yaml").string());
        if (!data["Scenes"] || data["Scenes"].size() == 0)
            throw std::runtime_error("No scenes in player manifest");
        WindowSpec spec{data["ProductName"].as<std::string>("Finality Game"), data["Width"].as<int>(1280),
                        data["Height"].as<int>(720), data["Fullscreen"].as<bool>(false)};
        int frames = argc == 3 && std::string(argv[1]) == "--smoke-frames" ? std::stoi(argv[2]) : 0;
        Application app;
        app.Initialize(RendererAPI::VULKAN, spec, std::make_unique<Player>(root, data, frames));
        app.Update();
        app.Shutdown();
        if (frames > 0)
            for (const auto& entry : Logger::GetConsoleSink()->CopyEntries())
                if (entry.Level >= spdlog::level::err) { std::cerr << entry.Message << '\n'; return 1; }
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Finality player: " << e.what() << '\n';
        return 1;
    }
}
