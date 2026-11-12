#pragma once
#include <Core/Process.h>
#include <Core/Log.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <future>
#include <sstream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>

namespace FINALITY
{
    class ExternalTools
    {
    public:
        inline static std::filesystem::path ProjectRoot;
        inline static std::string Executable;
        inline static std::vector<std::string> Installations;
        inline static std::string Status;
        inline static std::future<ProcessResult> Detection;
        inline static std::future<ProcessResult> SolutionGeneration;
        inline static std::filesystem::path PendingSource;
        inline static std::string ScriptDirectory = "Assets/Scripts";

        static void OpenProject(const std::filesystem::path& root, const std::string& scriptDirectory = "Assets/Scripts")
        {
            ProjectRoot = root;
            ScriptDirectory = scriptDirectory;
            Executable.clear();
            std::ifstream input(root / "Settings/ExternalTools.ini");
            input >> std::quoted(Executable);
            Detect();
        }

        static void Save()
        {
            std::filesystem::create_directories(ProjectRoot / "Settings");
            std::ofstream(ProjectRoot / "Settings/ExternalTools.ini") << std::quoted(Executable);
        }

        static void Detect()
        {
            if (Detection.valid()) return;
            const auto root = ProjectRoot;
            Detection = std::async(std::launch::async, [root]
            {
                const auto* directory = std::getenv("ProgramFiles(x86)");
                const auto locator = std::filesystem::path(directory ? directory : "C:/Program Files (x86)") / "Microsoft Visual Studio/Installer/vswhere.exe";
                return Process::Run({locator.wstring(), L"-all", L"-prerelease", L"-legacy", L"-products", L"*", L"-sort", L"-property", L"installationPath"}, root);
            });
        }

        static void Update()
        {
            if (SolutionGeneration.valid() && SolutionGeneration.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                const auto result = SolutionGeneration.get();
                const auto source = std::exchange(PendingSource, {});
                if (result.ExitCode == 0) OpenScript(source);
                else { Status = result.Output; Logger::GetClientLogger()->error("Solution generation failed: {}", Status); }
            }
            if (!Detection.valid() || Detection.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
            const auto result = Detection.get();
            Installations.clear();
            std::istringstream lines(result.Output);
            std::string line;
            while (std::getline(lines, line))
            {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                const auto executable = std::filesystem::path(line) / "Common7/IDE/devenv.exe";
                if (std::filesystem::is_regular_file(executable)) Installations.push_back(executable.string());
            }
            if (Executable.empty() && !Installations.empty()) { Executable = Installations.front(); Save(); }
            Status = result.ExitCode == 0 ? "Visual Studio discovery complete" : "Visual Studio discovery unavailable; select an executable manually";
        }

        static bool OpenScript(const std::filesystem::path& source)
        {
            if (!std::filesystem::is_regular_file(source)) return false;
            const auto file = std::filesystem::absolute(source).wstring();
            if (Executable.empty()) { Status = "Select a script editor under Project Settings > External Tools"; return false; }
            const std::filesystem::path executable(Executable);
            const auto solution = ProjectRoot / ".Finality/NativeScripts/FinalityProjectScripts.slnx";
            if (!std::filesystem::is_regular_file(solution) || !std::filesystem::is_regular_file(solution.parent_path() / "Finality Engine Sources.vcxproj"))
            {
                if (SolutionGeneration.valid()) return false;
                auto engine = Process::ExecutablePath().parent_path();
                while (!engine.empty() && !std::filesystem::is_regular_file(engine / "Scripts/NativeModule.ps1"))
                { const auto parent = engine.parent_path(); if (parent == engine) { engine.clear(); break; } engine = parent; }
                if (engine.empty()) { Status = "Native script tools were not found"; return false; }
                PendingSource = source;
                const auto root = ProjectRoot;
                const auto bin = Process::ExecutablePath().parent_path();
                const auto scripts = ScriptDirectory;
                SolutionGeneration = std::async(std::launch::async,[engine,root,bin,scripts]
                {
#ifdef NDEBUG
                    const std::wstring configuration = L"Release";
#else
                    const std::wstring configuration = L"Debug";
#endif
                    return Process::Run({L"powershell.exe",L"-NoProfile",L"-NonInteractive",L"-ExecutionPolicy",L"RemoteSigned",L"-File",
                        (engine / "Scripts/NativeModule.ps1").wstring(),L"-ProjectRoot",root.wstring(),L"-EngineRoot",engine.wstring(),L"-EngineBin",bin.wstring(),L"-ScriptDirectory",std::filesystem::path(scripts).wstring(),L"-Configuration",configuration,L"-GenerateOnly"},root);
                });
                Status = "Generating engine-linked solution";
                return true;
            }
            const auto arguments = executable.filename() == "devenv.exe" ?
                L"\"" + solution.wstring() + L"\" /edit \"" + file + L"\"" : L"\"" + ProjectRoot.wstring() + L"\" \"" + file + L"\"";
            const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", executable.c_str(), arguments.c_str(), ProjectRoot.c_str(), SW_SHOWNORMAL));
            Status = result > 32 ? "Opened script in external editor" : "Could not launch the selected external editor";
            if (result <= 32) Logger::GetClientLogger()->error("{}", Status);
            return result > 32;
        }
    };
}
