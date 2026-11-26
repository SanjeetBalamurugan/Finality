#include <Core/Process.h>
#include "ExecutableResources.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <thread>
#include <yaml-cpp/yaml.h>
namespace fs = std::filesystem;

void Run(const std::vector<std::wstring>& args, const fs::path& cwd)
{
    auto result = FINALITY::Process::Run(args, cwd);
    std::cout << result.Output << std::flush;
    if (result.ExitCode)
    {
        throw std::runtime_error("Build command failed (" + std::to_string(result.ExitCode) + ")");
    }
}

bool Inside(const fs::path& path, const fs::path& parent)
{
    auto relative = fs::weakly_canonical(path).lexically_relative(fs::weakly_canonical(parent));
    return !relative.empty() && *relative.begin() != "..";
}

int main(int argc, char** argv)
{
    fs::path stage;
    try
    {
        std::map<std::string, std::string> args;
        for (int i = 1; i + 1 < argc; i += 2)
        {
            args[argv[i]] = argv[i + 1];
        }
        constexpr const char* requiredOptions[] =
        {
            "--request",
            "--engine",
            "--bin"
        };

        for (auto option : requiredOptions)
        {
            if (!args.count(option))
            {
                throw std::runtime_error(std::string("Missing ") + option);
            }
        }
        auto request = YAML::LoadFile(args["--request"]);
        if (request["Platform"].as<std::string>() != "Windows")
        {
            throw std::runtime_error("Only Windows x64 is supported");
        }
        auto project = fs::canonical(request["ProjectRoot"].as<std::string>()),
             engine = fs::canonical(args["--engine"]), bin = fs::canonical(args["--bin"]),
             output = fs::absolute(request["Output"].as<std::string>()).lexically_normal();
        const auto buildInfo = YAML::LoadFile((bin / "FinalityBuild.json").string());
        const auto configuration = buildInfo["Configuration"].as<std::string>();
        if (configuration != "Release" && configuration != "Dist")
        {
            throw std::runtime_error("Game export requires Release or Dist engine binaries");
        }
        if (Inside(project, output) || Inside(engine, output) || Inside(output, project / "Assets"))
        {
            throw std::runtime_error("Output must not contain the project/engine or be inside Assets");
        }
        if (fs::exists(output) && !fs::is_empty(output))
        {
            throw std::runtime_error("Choose a new or empty output directory");
        }
        auto scenes = request["Scenes"];
        std::set<fs::path> includedScenes;
        if (!scenes || !scenes.IsSequence() || scenes.size() == 0)
        {
            throw std::runtime_error("Enable at least one scene");
        }
        for (auto entry : scenes)
        {
            auto path = project / entry.as<std::string>();
            if (!Inside(path, project / "Assets") || !fs::is_regular_file(path))
            {
                throw std::runtime_error("Invalid scene: " + path.string());
            }
            includedScenes.insert(fs::weakly_canonical(path));
        }
        constexpr const char* dependencyFiles[] =
        {
            "FinalityPlayer.exe",
            "FinalityEngine.dll",
            "ImGui.dll"
        };

        for (auto file : dependencyFiles)
        {
            if (!fs::is_regular_file(bin / file))
            {
                throw std::runtime_error(std::string("Missing player dependency: ") + file);
            }
        }
        std::cout << "Compiling project scripts...\n"
                  << std::flush;
        Run({ (bin / "FIBuild.exe").wstring(), L"native", L"--project", project.wstring(), L"--name", fs::path(request["ProjectName"].as<std::string>(project.filename().string())).wstring(),
              L"--engine", engine.wstring(), L"--bin", bin.wstring(), L"--source",
              fs::path(request["ScriptDirectory"].as<std::string>("Assets/Scripts")).wstring(),
              L"--configuration", fs::path(configuration).wstring() },
            engine);
        stage = output.parent_path() /
                (".Finality-export-" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(stage / "Data/Assets");
        fs::copy_file(bin / "FinalityPlayer.exe", stage / "Game.exe");
        constexpr const char* engineLibraries[] =
        {
            "FinalityEngine.dll",
            "ImGui.dll"
        };

        for (auto file : engineLibraries)
        {
            fs::copy_file(bin / file, stage / file);
        }
        fs::copy_file(project / ".Finality/NativeScripts/bin" / configuration / "GameScripts.dll",
                      stage / "GameScripts.dll");
        if (request["CopyPDB"].as<bool>(false))
        {
            constexpr const char* dependencyFiles3[] =
            {
                "FinalityPlayer.pdb",
                "FinalityEngine.pdb"
            };

            for (auto file : dependencyFiles3)
            {
                if (fs::exists(bin / file))
                {
                    fs::copy_file(bin / file, stage / file);
                }
            }
        }
        std::cout << "Copying assets...\n"
                  << std::flush;
        for (auto& file : fs::recursive_directory_iterator(project / "Assets"))
        {
            if (!file.is_regular_file())
            {
                continue;
            }
            const auto extension = file.path().extension();
            auto relative = file.path().lexically_relative(project / "Assets").generic_string();
            std::transform(relative.begin(), relative.end(), relative.begin(), [](unsigned char c)
                           {
                               return char(std::tolower(c));
                           });
            if (relative.starts_with("shaders/editor_grid.") || relative.starts_with("shaders/templates/"))
            {
                continue;
            }
            if (extension == ".fscene" && !includedScenes.count(fs::weakly_canonical(file.path())))
            {
                continue;
            }
            if (extension == ".log" || extension == ".vcxproj" || extension == ".user" || extension == ".sln" || extension == ".slnx")
            {
                continue;
            }
            auto dest = stage / "Data/Assets" / file.path().lexically_relative(project / "Assets");
            fs::create_directories(dest.parent_path());
            fs::copy_file(file.path(), dest);
        }
        auto compiler = bin / "glslc.exe";
        if (!fs::is_regular_file(compiler))
        {
            if (const char* sdk = std::getenv("VULKAN_SDK"))
            {
                compiler = fs::path(sdk) / "Bin/glslc.exe";
            }
        }
        if (!fs::is_regular_file(compiler))
        {
            throw std::runtime_error("glslc.exe is missing; install Vulkan SDK on the build machine");
        }
        std::cout << "Compiling shaders...\n"
                  << std::flush;
        fs::create_directories(stage / "Data/Builtin/shaders");
        constexpr const char* shaderFiles[] =
        {
            "imported_model.vert",
            "imported_model.frag"
        };

        for (auto shader : shaderFiles)
        {
            fs::copy_file(bin / "assets/shaders" / shader, stage / "Data/Builtin/shaders" / shader);
        }
        std::vector<fs::path> shaderSources;
        for (auto& file : fs::recursive_directory_iterator(stage / "Data"))
        {
            auto e = file.path().extension().string();
            std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c)
                           {
                               return char(std::tolower(c));
                           });
            if (e == ".vert" || e == ".frag" || e == ".comp" || e == ".geom" || e == ".tesc" || e == ".tese")
            {
                Run({ compiler.wstring(), L"-fshader-stage=" + fs::path(e.substr(1)).wstring(), L"-I", file.path().parent_path().wstring(),
                      file.path().wstring(), L"-o", file.path().wstring() + L".spv" },
                    engine);
                shaderSources.push_back(file.path());
            }
        }
        for (const auto& source : shaderSources)
        {
            fs::remove(source);
        }
        std::vector<fs::path> includes;
        const std::set<std::string> sourceExtensions{
            ".vert", ".frag", ".comp", ".geom", ".tesc", ".tese", ".glsl", ".hlsl", ".inc", ".shader", ".c", ".cc", ".cxx", ".cpp", ".h", ".hpp", ".inl", ".ipp", ".cs"
        };
        for (const auto& file : fs::recursive_directory_iterator(stage / "Data"))
        {
            auto extension = file.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
                           {
                               return char(std::tolower(c));
                           });
            if (file.is_regular_file() && sourceExtensions.contains(extension))
            {
                includes.push_back(file.path());
            }
        }
        for (const auto& include : includes)
        {
            fs::remove(include);
        }
        FINALITY::PlayerRuntime runtime;
        runtime.Width = request["Width"].as<int>(1280);
        runtime.Height = request["Height"].as<int>(720);
        runtime.Fullscreen = request["Fullscreen"].as<bool>(false);
        for (const auto& scene : scenes)
        {
            runtime.Scenes.push_back(scene.as<std::string>());
        }
        if (fs::is_regular_file(project / "Settings/Project.yaml"))
        {
            auto settings = YAML::LoadFile((project / "Settings/Project.yaml").string());
            if (const auto models = settings["Models"])
            {
                runtime.Models.Scale = models["Scale"].as<float>(1);
                runtime.Models.ConvertUnits = models["ConvertUnits"].as<bool>(true);
                runtime.Models.GenerateNormals = models["GenerateNormals"].as<bool>(true);
                runtime.Models.FlipUVs = models["FlipUVs"].as<bool>(false);
                runtime.Models.ImportMaterials = models["ImportMaterials"].as<bool>(true);
            }
        }
        BakeExecutableResources(stage / "Game.exe", project, bin, request, runtime);
        fs::copy_file(engine / "FIScriptReflect/licenses/LLVM-LICENSE.txt", stage / "LLVM-LICENSE.txt");
        std::ofstream(stage / "README.txt")
            << "Launch Game.exe. Requires Windows x64, Vulkan-capable graphics drivers and the Microsoft "
               "Visual C++ x64 runtime. Runtime settings and executable identity are embedded in Game.exe.\n";
        if (fs::exists(output))
        {
            fs::remove(output);
        }
        std::error_code renameError;
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            fs::rename(stage, output, renameError);
            if (!renameError)
            {
                break;
            }
            if (renameError != std::errc::permission_denied && renameError != std::errc::device_or_resource_busy)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (renameError)
        {
            throw fs::filesystem_error("Cannot finalize exported player", stage, output, renameError);
        }
        stage.clear();
        std::cout << "Build succeeded: " << output.string() << '\n';
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FIBaker: " << e.what() << '\n';
        if (!stage.empty())
        {
            std::error_code error;
            fs::remove_all(stage, error);
        }
        return 1;
    }
}
