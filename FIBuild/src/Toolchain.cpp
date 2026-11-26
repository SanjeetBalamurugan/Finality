#include "BuildSupport.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace FI::Build
{
    static fs::path ProgramFiles()
    {
        const auto* value = std::getenv("ProgramFiles(x86)");
        if (!value)
        {
            throw std::runtime_error("Windows C++ toolchain discovery requires ProgramFiles(x86)");
        }
        return Path(value);
    }

    Toolchain FindToolchain(const fs::path& engine, const fs::path& bin, const std::string& config)
    {
        const auto manifest = bin / "FinalityBuild.json";
        if (!fs::is_regular_file(manifest))
        {
            throw std::runtime_error("Rebuild FinalityEngine: engine build information is missing");
        }
        const auto info = YAML::Load(Read(manifest));
        if (info["Configuration"].as<std::string>() != config)
        {
            throw std::runtime_error("Script and engine configurations must match");
        }
        if (info["EngineHash"].as<std::string>() != Hash(Read(bin / "FinalityEngine.dll")))
        {
            throw std::runtime_error("Engine binary differs from its build information; rebuild FinalityEngine");
        }
        if (info["HeaderHash"].as<std::string>() != HeaderHash(engine))
        {
            throw std::runtime_error("Engine headers changed; rebuild FinalityEngine before compiling scripts");
        }
        Toolchain result;
        result.Version = info["VCToolsVersion"].as<std::string>();
        result.Toolset = info["PlatformToolset"].as<std::string>();
        result.SDKVersion = info["WindowsSdkVersion"].as<std::string>();
        const auto programs = ProgramFiles();
        const auto installations = YAML::Load(
            Run({ Text(programs / "Microsoft Visual Studio/Installer/vswhere.exe"), "-all", "-products", "*",
                  "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-format", "json" },
                engine));
        for (const auto& item : installations)
        {
            const auto installation = Path(item["installationPath"].as<std::string>());
            result.Compiler = installation / "VC/Tools/MSVC" / result.Version;
            if (!fs::is_regular_file(result.Compiler / "bin/Hostx64/x64/cl.exe"))
            {
                continue;
            }
            result.SDK = programs / "Windows Kits/10/Include" / result.SDKVersion;
            if (!fs::is_regular_file(result.SDK / "um/Windows.h"))
            {
                throw std::runtime_error("Install matching Windows SDK " + result.SDKVersion);
            }
            result.BuildTool = installation / "MSBuild/Current/Bin/amd64/MSBuild.exe";
            if (!fs::is_regular_file(result.BuildTool))
            {
                result.BuildTool = installation / "MSBuild/Current/Bin/MSBuild.exe";
            }
            result.Action = std::stoi(item["installationVersion"].as<std::string>()) >= 18 ? "vs2026" : "vs2022";
            return result;
        }
        throw std::runtime_error("Install matching MSVC " + result.Version +
                                 ", or rebuild the engine with an installed compiler");
    }

    void BuildInfo(const Options& options)
    {
        const auto engine = fs::absolute(Path(options.at("--engine"))), bin = fs::absolute(Path(options.at("--bin")));
        auto sdk = options.at("--sdk-version");
        if (sdk == "10.0")
        {
            for (const auto& entry : fs::directory_iterator(ProgramFiles() / "Windows Kits/10/Include"))
            {
                if (entry.is_directory() && Text(entry.path().filename()) > sdk)
                {
                    sdk = Text(entry.path().filename());
                }
            }
        }
        YAML::Node info;
        info["Version"] = 1;
        info["Version"].SetTag("!!int");
        info["Configuration"] = options.at("--configuration");
        info["VCToolsVersion"] = options.at("--compiler-version");
        info["PlatformToolset"] = options.at("--toolset");
        info["WindowsSdkVersion"] = sdk;
        info["HeaderHash"] = HeaderHash(engine);
        info["EngineHash"] = Hash(Read(bin / "FinalityEngine.dll"));
        Write(bin / "FinalityBuild.json", Json(info));
    }

    Toolchain CheckNative(const Options& options)
    {
        const auto engine = fs::absolute(Path(options.at("--engine"))), bin = fs::absolute(Path(options.at("--bin")));
        constexpr const char* requiredFiles[] =
        {
            "FinalityEngine/src/Scene/ScriptableComponent.h",
            "Scripts/Templates/ProjectScripts.lua",
            "Scripts/Templates/ProjectSpin.legacy-hash",
            "Scripts/NoVisualStudioFilters.lua",
            "vendor/premake/premake5.exe",
            "vendor/glm/glm/glm.hpp",
            "vendor/entt/include/entt.hpp",
            "vendor/spdlog/include/spdlog/spdlog.h",
            "vendor/yaml-cpp/include/yaml-cpp/yaml.h",
            "vendor/imgui/imgui.h"
        };

        for (const auto* required : requiredFiles)
        {
            if (!fs::is_regular_file(engine / required))
            {
                throw std::runtime_error(std::string("Native development file missing: ") + required +
                                         ". Restore the matching engine development package and initialize its dependencies.");
            }
        }
        constexpr const char* requiredTools[] =
        {
            "FinalityEngine.lib",
            "FIScriptReflect.exe",
            "libclang.dll",
            "ClangResource/include/stddef.h"
        };

        for (const auto* required : requiredTools)
        {
            if (!fs::is_regular_file(bin / required))
            {
                throw std::runtime_error(std::string("Native scripting tool missing: ") + required +
                                         ". Restore the complete editor development package or rebuild it.");
            }
        }
        return FindToolchain(engine, bin, options.contains("--configuration") ? options.at("--configuration") : "Release");
    }

    void Dependencies(const Options& options)
    {
        const auto engine = fs::absolute(Path(options.at("--engine")));
        const auto output = engine / "bin-int/Assimp";
        std::vector<std::string> args{ "cmake", "-S", Text(engine / "vendor/assimp"), "-B", Text(output) };
#ifdef _WIN32
        const auto programs = ProgramFiles();
        const auto installations = YAML::Load(
            Run({ Text(programs / "Microsoft Visual Studio/Installer/vswhere.exe"), "-all", "-products", "*",
                  "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-format", "json" },
                engine));
        if (!installations.IsSequence() || installations.size() == 0)
        {
            throw std::runtime_error("Install Visual Studio C++ tools");
        }
        size_t selectedIndex = 0;
        if (options.contains("--compiler-version"))
        {
            bool found = false;
            for (size_t index = 0; index < installations.size(); ++index)
            {
                const auto item = installations[index];
                const auto compiler = Path(item["installationPath"].as<std::string>()) / "VC/Tools/MSVC" /
                                      options.at("--compiler-version") / "bin/Hostx64/x64/cl.exe";
                if (!fs::is_regular_file(compiler))
                {
                    continue;
                }
                selectedIndex = index;
                found = true;
                break;
            }
            if (!found)
            {
                throw std::runtime_error("Install MSVC " + options.at("--compiler-version") + " to build Assimp with the engine's compiler");
            }
        }
        const auto selected = installations[selectedIndex];
        const auto major = std::stoi(selected["installationVersion"].as<std::string>());
        const auto installation = Path(selected["installationPath"].as<std::string>());
        // Prefer Visual Studio's CMake so a clean build does not require a separate PATH setup.
        const auto visualStudioCMake = installation / "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe";
        if (fs::is_regular_file(visualStudioCMake))
        {
            args[0] = Text(visualStudioCMake);
        }
        else if (const auto* programs64 = std::getenv("ProgramFiles"))
        {
            const auto installedCMake = Path(programs64) / "CMake/bin/cmake.exe";
            if (fs::is_regular_file(installedCMake))
            {
                args[0] = Text(installedCMake);
            }
        }
        auto version = Read(installation / "VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt");
        while (!version.empty() && std::isspace(static_cast<unsigned char>(version.back())))
        {
            version.pop_back();
        }
        if (options.contains("--compiler-version"))
        {
            version = options.at("--compiler-version");
        }
        const auto toolset = (options.contains("--toolset") ? options.at("--toolset") : std::string(major >= 18 ? "v145" : "v143")) + ",version=" + version;
        const auto cache = output / "CMakeCache.txt";
        if (fs::is_regular_file(cache) && Read(cache).find("CMAKE_GENERATOR_TOOLSET:INTERNAL=" + toolset) == std::string::npos)
        {
            if (!Inside(output, engine))
            {
                throw std::runtime_error("Dependency output resolved outside the engine workspace");
            }
            const auto generated = output / "CMakeFiles";
            if (fs::exists(generated) && !Inside(generated, output))
            {
                throw std::runtime_error("CMake configuration resolved outside dependency output");
            }
            fs::remove(cache);
            if (fs::exists(generated))
            {
                fs::remove_all(generated);
            }
            std::cout << "Refreshing generated CMake configuration for installed MSVC " << version << ".\n";
        }
        args.insert(args.end(),
                    { "-G", major >= 18 ? "Visual Studio 18 2026" : "Visual Studio 17 2022", "-A", "x64", "-T",
                      toolset, "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" });
#endif
        constexpr const char* requiredOptions[] =
        {
            "BUILD_SHARED_LIBS=OFF",
            "ASSIMP_BUILD_TESTS=OFF",
            "ASSIMP_BUILD_ASSIMP_TOOLS=OFF",
            "ASSIMP_BUILD_SAMPLES=OFF",
            "ASSIMP_INSTALL=OFF",
            "ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF",
            "ASSIMP_BUILD_OBJ_IMPORTER=ON",
            "ASSIMP_BUILD_GLTF_IMPORTER=ON",
            "ASSIMP_BUILD_FBX_IMPORTER=ON",
            "ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=OFF",
            "ASSIMP_BUILD_ZLIB=ON",
            "ASSIMP_BUILD_DRACO=OFF",
            "ASSIMP_INJECT_DEBUG_POSTFIX=OFF",
            "LIBRARY_SUFFIX="
        };

        for (const auto* option : requiredOptions)
        {
            args.push_back(std::string("-D") + option);
        }
        std::cout << Run(args, engine);
        const auto config = options.contains("--configuration") ? options.at("--configuration") : "Release";
        std::cout << Run({ args[0], "--build", Text(output), "--config", config == "Dist" ? "Release" : config, "--parallel", "4" }, engine);
    }
} // namespace FI::Build
