#include "BuildSupport.h"
#include <algorithm>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

namespace FI::Build
{
    static std::vector<fs::path> Files(const fs::path& root)
    {
        const std::set<std::string> excluded{ ".git", ".vs", ".Finality", "bin",
                                              "bin-int", "obj", "Builds", "node_modules" };
        const std::set<std::string> binary{ ".sln", ".slnx", ".vcxproj", ".filters", ".user", ".code-workspace",
                                            ".log", ".spv", ".dll", ".exe", ".pdb", ".lib",
                                            ".obj", ".vcxproj" };
        const std::set<std::string> known{
            ".cpp", ".c", ".cc", ".cxx", ".h", ".hpp", ".hxx", ".inl", ".ipp", ".cs",
            ".lua", ".py", ".sh", ".bat", ".cmd", ".txt", ".md", ".dox", ".toml", ".ini",
            ".cfg", ".json", ".xml", ".yaml", ".yml", ".fproj", ".fscene", ".fmat", ".fprefab", ".fmeta",
            ".vert", ".frag", ".glsl", ".comp", ".hlsl", ".shader", ".cmake", ".props", ".targets", ".png",
            ".jpg", ".jpeg", ".bmp", ".ico", ".svg", ".dds", ".tga"
        };
        std::vector<fs::path> result;
        for (fs::recursive_directory_iterator it(root), end; it != end; ++it)
        {
            if (it->is_symlink())
            {
                it.disable_recursion_pending();
                continue;
            }
            if (it->is_directory())
            {
                if (excluded.contains(Text(it->path().filename())))
                {
                    it.disable_recursion_pending();
                }
                continue;
            }
            if (!it->is_regular_file())
            {
                continue;
            }
            auto extension = Text(it->path().extension());
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c)
                           {
                               return char(std::tolower(c));
                           });
            if (binary.contains(extension))
            {
                continue;
            }
            bool include = known.contains(extension);
            if (!include && it->file_size() <= 64 * 1024 * 1024)
            {
                std::ifstream stream(it->path(), std::ios::binary);
                std::string sample(8192, '\0');
                stream.read(sample.data(), sample.size());
                sample.resize(size_t(stream.gcount()));
                include = std::all_of(sample.begin(), sample.end(),
                                      [](unsigned char c)
                                      {
                                          return c >= 32 || c == 9 || c == 10 || c == 13;
                                      });
            }
            if (include)
            {
                result.push_back(it->path());
            }
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    static std::string Unxml(std::string text)
    {
        const std::pair<std::string, std::string> replacements[] =
        {
            { "&quot;", "\"" },
            { "&lt;", "<" },
            { "&gt;", ">" },
            { "&amp;", "&" }
        };

        for (const auto& [from, to] : replacements)
        {
            for (size_t pos = 0; (pos = text.find(from, pos)) != std::string::npos; pos += to.size())
            {
                text.replace(pos, from.size(), to);
            }
        }
        return text;
    }

    void GenerateIDE(const fs::path& root, const fs::path& engine, const fs::path& bin, const fs::path& source,
                     const std::string& name, const std::string& config, const Toolchain& tools)
    {
        const auto directory = root / ".Finality/NativeScripts", project = root / "GameScripts.vcxproj";
        auto xml = Read(project);
        std::set<fs::path> existing;
        const std::regex item("<(?:ClCompile|ClInclude|None|ResourceCompile|Image|Text|CustomBuild|FxCompile) "
                              "Include=\"([^\"]+)\"");
        for (std::sregex_iterator it(xml.begin(), xml.end(), item), end; it != end; ++it)
        {
            const auto value = Unxml((*it)[1].str());
            if (value.find_first_of("$*?;") != std::string::npos)
            {
                continue;
            }
            const auto path = Path(value);
            existing.insert(fs::absolute(path.is_absolute() ? path : root / path).lexically_normal());
        }
        const auto files = Files(root);
        std::string additions = "\n  <ItemGroup Label=\"FinalityProjectFiles\">\n", manifest;
        for (const auto& file : files)
        {
            manifest += Text(file) + "\n";
            if (!existing.contains(fs::absolute(file).lexically_normal()))
            {
                additions += "    <None Include=\"" + Xml(Text(file.lexically_relative(root))) + "\" />\n";
            }
        }
        additions += "  </ItemGroup>\n";
        const auto quote = [](const fs::path& p)
        {
            return "\"" + Text(p) + "\"";
        };
        const auto command = quote(bin / "FIScriptReflect.exe") + " --engine " + quote(engine) + " --source " + quote(source) + " --output " + quote(directory / "Reflected/$(Configuration)") + " --msvc " + quote(tools.Compiler) + " --sdk " + quote(tools.SDK) + " --configuration \"$(Configuration)\"";
        additions +=
            "  <Target Name=\"FinalityReflect\" BeforeTargets=\"PrepareForBuild\">\n    <Exec Command=\"" +
            Xml(command) +
            "\" Condition=\"'$(FinalitySkipReflection)' != 'true'\" />\n    <ItemGroup><ClCompile "
            "Include=\".Finality\\NativeScripts\\Reflected\\$(Configuration)\\*.cpp\" Visible=\"false\" /></ItemGroup>\n  </Target>\n";
        const auto close = xml.rfind("</Project>");
        if (close == std::string::npos)
        {
            throw std::runtime_error("Invalid generated script project");
        }
        xml.insert(close, additions);
        Write(project, xml);
        fs::remove(Path(Text(project) + ".filters"));
        // The project lives beside Assets, so Visual Studio's physical view shows the authored folders.
        const auto user = Path(Text(project) + ".user");
        if (!fs::exists(user))
        {
            Write(user, "<?xml version=\"1.0\" encoding=\"utf-8\"?><Project xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\"><PropertyGroup><ShowAllFiles>true</ShowAllFiles></PropertyGroup></Project>");
        }
        fs::remove(directory / "GameScripts.vcxproj");
        fs::remove(directory / "GameScripts.vcxproj.filters");
        Write(directory / "IDEFiles.txt", manifest);
        const auto ignore = engine / "Scripts/Templates/Project.gitignore";
        const auto ignoreText = Read(ignore);
        auto legacy = ignoreText;
        const std::pair<std::string, std::string> legacyReplacements[] =
        {
            { "/*.sln", "/FinalityProjectScripts.sln" },
            { "/*.code-workspace", "/Finality.code-workspace" }
        };

        for (const auto& [from, to] : legacyReplacements)
        {
            for (size_t pos = 0; (pos = legacy.find(from, pos)) != std::string::npos; pos += to.size())
            {
                legacy.replace(pos, from.size(), to);
            }
        }
        const auto normalized = [](std::string text)
        {
            text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
            {
                text.pop_back();
            }
            return text;
        };
        if (!fs::exists(root / ".gitignore") || normalized(Read(root / ".gitignore")) == normalized(legacy))
        {
            Write(root / ".gitignore", ignoreText);
        }
        else if (Read(root / ".gitignore").find("/GameScripts.vcxproj") == std::string::npos)
        {
            Write(root / ".gitignore", Read(root / ".gitignore") + "\n/GameScripts.vcxproj\n");
        }
        YAML::Node workspace;
        workspace["folders"][0]["name"] = name;
        workspace["folders"][0]["path"] = Text(root);
        workspace["folders"][1]["name"] = fs::is_regular_file(engine / "FinalitySDK.json") ? "Finality SDK" : "Finality Engine";
        workspace["folders"][1]["path"] = Text(engine / "FinalityEngine");
        auto settings = workspace["settings"];
        settings["C_Cpp.default.compilerPath"] = Text(tools.Compiler / "bin/Hostx64/x64/cl.exe");
        settings["C_Cpp.default.cppStandard"] = "c++20";
        settings["C_Cpp.default.intelliSenseMode"] = "windows-msvc-x64";
        settings["C_Cpp.default.windowsSdkVersion"] = tools.SDKVersion;
        const std::filesystem::path includeDirectories[] =
        {
            source,
            root / "Assets",
            engine / "FinalityEngine/src",
            engine / "vendor/glm",
            engine / "vendor/entt/include",
            engine / "vendor/spdlog/include",
            engine / "vendor/yaml-cpp/include",
            engine / "vendor/imgui"
        };

        for (const auto& include : includeDirectories)
        {
            settings["C_Cpp.default.includePath"].push_back(Text(include));
        }
        constexpr const char* defineValues[] =
        {
            "GLM_FORCE_PURE",
            "GLM_FORCE_UNRESTRICTED_FLOAT",
            "IMGUI_API=__declspec(dllimport)",
            "_MT",
            "_DLL"
        };

        for (const auto* define : defineValues)
        {
            settings["C_Cpp.default.defines"].push_back(define);
        }
        settings["C_Cpp.default.defines"].push_back(config != "Debug" ? "NDEBUG" : "_DEBUG");
        if (config == "Debug")
        {
            settings["C_Cpp.default.defines"].push_back("DEBUG");
        }
        constexpr const char* hiddenDirectories[] =
        {
            "**/.Finality",
            "**/.vs"
        };

        for (const auto* hidden : hiddenDirectories)
        {
            settings["files.exclude"][hidden] = true;
            settings["files.exclude"][hidden].SetTag("!!bool");
        }
        workspace["extensions"]["recommendations"].push_back("ms-vscode.cpptools");
        auto tasks = workspace["tasks"];
        tasks["version"] = "2.0.0";
        auto task = tasks["tasks"][0];
        task["label"] = "Finality: Build Scripts";
        task["type"] = "process";
        task["command"] = Text(bin / "FIBuild.exe");
        const std::string buildArguments[] =
        {
            "native",
            "--project",
            Text(root),
            "--name",
            name,
            "--engine",
            Text(engine),
            "--bin",
            Text(bin),
            "--source",
            Text(source.lexically_relative(root)),
            "--configuration",
            config
        };

        for (const auto& value : buildArguments)
        {
            task["args"].push_back(value);
        }
        task["problemMatcher"] = "$msCompile";
        Write(root / (name + ".code-workspace"), Json(workspace));
        const auto names = directory / "IDEProjectName.txt";
        if (fs::is_regular_file(names))
        {
            const auto previous = Read(names);
            if (previous != name && Name(previous) == previous)
            {
                constexpr const char* solutionExtensions[] =
                {
                    ".slnx",
                    ".sln",
                    ".code-workspace"
                };

                for (const auto* extension : solutionExtensions)
                {
                    fs::remove(root / (previous + extension));
                }
            }
        }
        constexpr const char* legacyFiles[] =
        {
            "FinalityProjectScripts.slnx",
            "FinalityProjectScripts.sln",
            "Finality.code-workspace"
        };

        for (const auto* old : legacyFiles)
        {
            if (std::string(old) != name + ".slnx" && std::string(old) != name + ".sln" && std::string(old) != name + ".code-workspace")
            {
                fs::remove(root / old);
            }
        }
        constexpr const char* legacyNativeSolutions[] =
        {
            "FinalityProjectScripts.slnx",
            "FinalityProjectScripts.sln"
        };

        for (const auto* old : legacyNativeSolutions)
        {
            fs::remove(directory / old);
        }
        Write(names, name);
    }
} // namespace FI::Build
