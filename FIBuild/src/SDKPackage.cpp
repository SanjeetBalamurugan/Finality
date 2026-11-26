#include "BuildSupport.h"
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>

namespace FI::Build
{
    void PackageSDK(const Options& options)
    {
        const auto engine = fs::canonical(Path(options.at("--engine")));
        const auto bin = fs::canonical(Path(options.at("--bin")));
        const auto output = fs::absolute(Path(options.at("--output"))).lexically_normal();

        if (!Inside(output, bin) || output == bin)
        {
            throw std::runtime_error(
                "The development SDK must be inside its matching editor binary directory");
        }

        if (fs::exists(output) && (!fs::is_regular_file(output / "FinalitySDK.json") || fs::is_symlink(output)))
        {
            throw std::runtime_error(
                "Refusing to replace a directory that is not a generated Finality SDK");
        }

        const auto stage = bin / (".SDK-package-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        try
        {
            auto copy = [&](const fs::path& source, const fs::path& relative)
            {
                const auto target = stage / relative;

                fs::create_directories(target.parent_path());
                fs::copy_file(source, target, fs::copy_options::overwrite_existing);
            };

            const std::set<std::string> extensions{
                ".h",
                ".hpp",
                ".hh",
                ".hxx",
                ".inl",
                ".ipp",
                ".tcc"
            };

            constexpr const char* directories[] =
            {
                "FinalityEngine/src",
                "vendor/glm",
                "vendor/entt/include",
                "vendor/spdlog/include",
                "vendor/yaml-cpp/include",
                "vendor/imgui"
            };

            for (const auto* directory : directories)
            {
                for (fs::recursive_directory_iterator it(engine / directory), end; it != end; ++it)
                {
                    if (it->is_symlink())
                    {
                        it.disable_recursion_pending();
                        continue;
                    }

                    if (it->is_directory() && it->path().filename() == ".git")
                    {
                        it.disable_recursion_pending();
                        continue;
                    }

                    if (it->is_regular_file() && extensions.contains(Text(it->path().extension())))
                    {
                        copy(it->path(), it->path().lexically_relative(engine));
                    }
                }

                for (const auto& file : fs::directory_iterator(engine / directory))
                {
                    if (file.is_regular_file() && (Text(file.path().filename()).starts_with("LICENSE") || Text(file.path().filename()).starts_with("COPYING")))
                    {
                        copy(file.path(), file.path().lexically_relative(engine));
                    }
                }
            }

            constexpr const char* requiredFiles[] =
            {
                "Scripts/Templates/ProjectScripts.lua",
                "Scripts/Templates/Project.gitignore",
                "Scripts/Templates/ProjectSpin.legacy-hash",
                "Scripts/NoVisualStudioFilters.lua",
                "vendor/premake/premake5.exe",
                "FIScriptReflect/licenses/LLVM-LICENSE.txt"
            };

            for (const auto* file : requiredFiles)
            {
                copy(engine / file, file);
            }

            constexpr const char* libraries[] =
            {
                "FinalityEngine.lib",
                "ImGui.lib"
            };

            for (const auto* library : libraries)
            {
                copy(bin / library, fs::path("lib") / library);
            }

            constexpr const char* packages[] =
            {
                "vendor/glm",
                "vendor/entt",
                "vendor/spdlog",
                "vendor/yaml-cpp",
                "vendor/imgui",
                "vendor/premake"
            };

            for (const auto* package : packages)
            {
                for (const auto& file : fs::directory_iterator(engine / package))
                {
                    if (file.is_regular_file() && (Text(file.path().filename()).starts_with("LICENSE") || Text(file.path().filename()).starts_with("COPYING")))
                    {
                        copy(file.path(), file.path().lexically_relative(engine));
                    }
                }
            }

            auto manifest = YAML::Load(Read(bin / "FinalityBuild.json"));

            if (manifest["Configuration"].as<std::string>() != "Dist")
            {
                throw std::runtime_error(
                    "The distribution SDK must be packaged from a Dist build");
            }

            manifest["SDKVersion"] = 1;
            manifest["BinaryDirectory"] = "..";

            Write(stage / "FinalitySDK.json", Json(manifest));

            if (HeaderHash(stage) != manifest["HeaderHash"].as<std::string>())
            {
                throw std::runtime_error("Packaged headers do not match the built engine");
            }

            Write(
                stage / "README.txt",
                "Finality development SDK. Native scripts use the editor binaries one directory above. "
                "Install the MSVC and Windows SDK versions recorded in FinalitySDK.json. "
                "Engine implementation sources and LLVM source are not required.\n");

            if (fs::exists(output))
            {
                fs::remove_all(output);
            }

            fs::rename(stage, output);

            std::cout << "Development SDK packaged: " << Text(output) << '\n';
        }
        catch (...)
        {
            std::error_code ignored;

            fs::remove_all(stage, ignored);
            throw;
        }
    }
} // namespace FI::Build
