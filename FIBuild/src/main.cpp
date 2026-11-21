#include "BuildSupport.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace FI::Build
{
void Native(const Options &options)
{
    const auto engine = fs::canonical(Path(options.at("--engine"))),
               bin = fs::canonical(Path(options.at("--bin"))),
               root = fs::canonical(Path(options.at("--project")));
    const auto config = options.contains("--configuration") ? options.at("--configuration") : "Release";
    if (config != "Release" && config != "Debug")
        throw std::runtime_error("Configuration must be Debug or Release");
    auto projectName = Text(root.filename());
    if (!options.contains("--name"))
        for (const auto &entry : fs::directory_iterator(root))
            if (entry.is_regular_file() && entry.path().extension() == ".fproj")
            {
                const auto descriptor = YAML::Load(Read(entry.path()));
                projectName = descriptor["Project"]["Name"].as<std::string>(Text(entry.path().stem()));
                break;
            }
    auto name = Name(options.contains("--name") ? options.at("--name") : projectName);
    auto source =
        fs::absolute(root / Path(options.contains("--source") ? options.at("--source") : "Assets/Scripts"))
            .lexically_normal();
    if (!Inside(source, root) || source == root)
        throw std::runtime_error("Script source must stay inside the project");
    const auto assets = root / "Assets";
    if (Inside(source, assets))
        source = assets;
    fs::create_directories(source);
    const auto tools = FindToolchain(engine, bin, config);
    std::cout << "Building scripts with MSVC " << tools.Version << " and SDK " << tools.SDKVersion
              << ", matching FinalityEngine.dll.\n";
#ifdef _WIN32
    const auto *previous = std::getenv("PATH");
    const auto environment = Text(tools.Compiler / "bin/Hostx64/x64") + ";" + (previous ? previous : "");
    _putenv_s("PATH", environment.c_str());
#endif
    auto expected = Read(engine / "Scripts/Templates/ProjectSpin.legacy-hash");
    expected.erase(
        std::remove_if(expected.begin(), expected.end(), [](unsigned char c) { return std::isspace(c); }),
        expected.end());
    for (const auto &entry : fs::recursive_directory_iterator(source))
        if (entry.is_regular_file() && entry.path().filename() == "ProjectScripts.cpp")
        {
            auto text = Read(entry.path());
            text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
            const auto first = text.find_first_not_of(" \t\n"), last = text.find_last_not_of(" \t\n");
            const auto normalized = first == std::string::npos ? "" : text.substr(first, last - first + 1);
            if (Hash(normalized) == expected)
                Write(entry.path(), "#include <Scene/ScriptModule.h>\n");
        }
    const auto reflected = root / ".Finality/NativeScripts/Reflected" / config;
    for (const auto *required : {"FIScriptReflect.exe", "libclang.dll", "ClangResource/include/stddef.h"})
        if (!fs::is_regular_file(bin / required))
            throw std::runtime_error(std::string("Reflection package missing ") + required);
    if (!options.contains("--generate-only"))
        std::cout << Run({Text(bin / "FIScriptReflect.exe"), "--engine", Text(engine), "--source",
                          Text(source), "--output", Text(reflected), "--msvc", Text(tools.Compiler), "--sdk",
                          Text(tools.SDK), "--configuration", config},
                         root);
    else
        fs::create_directories(reflected);
    std::cout << Run({Text(engine / "vendor/premake/premake5.exe"),
                      "--file=" + Text(engine / "Scripts/Templates/ProjectScripts.lua"), tools.Action,
                      "--project-name=" + name, "--engine-root=" + Text(engine), "--engine-bin=" + Text(bin),
                      "--project-root=" + Text(root), "--script-directory=" + Text(source),
                      "--reflected-directory=" + Text(reflected), "--native-toolset=" + tools.Toolset,
                      "--native-version=" + tools.Version, "--windows-sdk=" + tools.SDKVersion},
                     root);
    GenerateIDE(root, engine, bin, source, name, config, tools);
    if (options.contains("--generate-only"))
        return;
    std::cout << Run({Text(tools.BuildTool), Text(root / ".Finality/NativeScripts/GameScripts.vcxproj"),
                      "/m:1", "/nr:false", "/p:BuildProjectReferences=false",
                      "/p:FinalitySkipReflection=true", "/p:Configuration=" + config, "/p:Platform=x64",
                      "/p:PreferredToolArchitecture=x64", "/p:VCToolsVersion=" + tools.Version,
                      "/p:PlatformToolset=" + tools.Toolset,
                      "/p:WindowsTargetPlatformVersion=" + tools.SDKVersion, "/v:minimal", "/nologo"},
                     root);
}
}
int BuildMain(const std::vector<std::string> &argv)
{
    using namespace FI::Build;
    try
    {
        if (argv.size() < 2)
            throw std::runtime_error("Usage: FIBuild <native|info|dependencies|hash> --option value");
        Options options;
        for (size_t i = 2; i < argv.size(); ++i)
        {
            const std::string key = argv[i];
            if (key == "--generate-only")
            {
                options[key] = "true";
                continue;
            }
            if (i + 1 >= argv.size())
                throw std::runtime_error("Missing value for " + key);
            options[key] = argv[++i];
        }
        const std::string command = argv[1];
        const std::map<std::string, std::vector<std::string>> required{
            {"native", {"--project", "--engine", "--bin"}},
            {"info",
             {"--engine", "--bin", "--compiler-version", "--toolset", "--sdk-version", "--configuration"}},
            {"dependencies", {"--engine"}},
            {"hash", {}}};
        if (!required.contains(command))
            throw std::runtime_error("Unknown build command: " + command);
        for (const auto &key : required.at(command))
            if (!options.contains(key))
                throw std::runtime_error("Missing required option " + key);
        if (command == "hash" && options.contains("--file") == options.contains("--engine"))
            throw std::runtime_error("Choose exactly one of --file or --engine for hashing");
        if (command == "native")
            Native(options);
        else if (command == "info")
            BuildInfo(options);
        else if (command == "dependencies")
            Dependencies(options);
        else if (command == "hash")
            std::cout << (options.contains("--file") ? Hash(Read(Path(options.at("--file"))))
                                                     : HeaderHash(Path(options.at("--engine"))))
                      << '\n';
        else
            throw std::runtime_error("Unknown build command: " + command);
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FIBuild: " << error.what() << '\n';
        return 1;
    }
}
#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
{
    std::vector<std::string> values;
    for (int i = 0; i < argc; ++i)
    {
        const auto size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0)
            return 1;
        std::string text(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, text.data(), size, nullptr, nullptr);
        text.pop_back();
        values.push_back(std::move(text));
    }
    return BuildMain(values);
}
#else
int main(int argc, char **argv)
{
    return BuildMain(std::vector<std::string>(argv, argv + argc));
}
#endif
