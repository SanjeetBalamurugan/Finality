/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ExternalTools.h.
 */
#pragma once
#include "VisualStudioIntegration.h"
#include "DevelopmentSDK.h"
#include <Core/Log.h>
#include <Core/Process.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <sstream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>

namespace FINALITY
{
    /** @todo TODO: Document ExternalTools class semantics, ownership and usage. */
    class ExternalTools
    {
    public:
        std::filesystem::path ProjectRoot;
        std::string Executable;
        std::string ProjectName;
        std::vector<std::string> Installations;
        std::string Status;
        std::future<ProcessResult> Detection;
        std::future<ProcessResult> SolutionGeneration;
        std::filesystem::path PendingSource;
        std::filesystem::path PendingVisualStudioSource, PendingVisualStudioSolution;
        std::chrono::steady_clock::time_point VisualStudioDeadline{}, VisualStudioRetry{};
        std::string ScriptDirectory = "Assets/Scripts";

        static std::string SafeName(std::string name)
        {
            for (auto& c : name)
            {
                if (static_cast<unsigned char>(c) < 32 || std::string("<>:\"/\\|?*").find(c) != std::string::npos)
                {
                    c = '_';
                }
            }
            while (!name.empty() && name.front() == ' ')
            {
                name.erase(name.begin());
            }
            while (!name.empty() && (name.back() == ' ' || name.back() == '.'))
            {
                name.pop_back();
            }
            return name.empty() ? "FinalityProject" : name;
        }

        static std::wstring QuoteArgument(const std::wstring& value)
        {
            std::wstring result = L"\"";
            size_t slashes = 0;
            for (auto c : value)
            {
                if (c == L'\\')
                {
                    ++slashes;
                    continue;
                }
                result.append(c == L'\"' ? slashes * 2 + 1 : slashes, L'\\');
                result += c;
                slashes = 0;
            }
            result.append(slashes * 2, L'\\');
            return result + L"\"";
        }

        static std::wstring ScriptArguments(const std::filesystem::path& executable,
                                            const std::filesystem::path& root,
                                            const std::filesystem::path& solution,
                                            const std::filesystem::path& source)
        {
            const auto name = executable.filename().wstring();
            const auto file = std::filesystem::absolute(source).wstring();
            if (_wcsicmp(name.c_str(), L"devenv.exe") == 0)
            {
                return QuoteArgument(solution.wstring());
            }
            if (_wcsicmp(name.c_str(), L"Code.exe") == 0 || _wcsicmp(name.c_str(), L"Code - Insiders.exe") == 0)
            {
                return L"--reuse-window \"" +
                       (root / (solution.stem().string() + ".code-workspace")).wstring() + L"\" --goto \"" +
                       file + L":1\"";
            }
            return L"\"" + root.wstring() + L"\" \"" + file + L"\"";
        }

        void CloseProject()
        {
            if (SolutionGeneration.valid())
            {
                SolutionGeneration.get();
            }
            if (Detection.valid())
            {
                Detection.get();
            }
            PendingSource.clear();
            PendingVisualStudioSource.clear();
            PendingVisualStudioSolution.clear();
            ProjectRoot.clear();
            ProjectName.clear();
            Executable.clear();
            Installations.clear();
            Status.clear();
        }

        void OpenProject(const std::filesystem::path& root,
                         const std::string& scriptDirectory = "Assets/Scripts", const std::string& name = {})
        {
            ProjectRoot = root;
            ProjectName = SafeName(name.empty() ? root.filename().string() : name);
            ScriptDirectory = scriptDirectory;
            Executable.clear();
            std::ifstream input(root / "Settings/ExternalTools.ini");
            input >> std::quoted(Executable);
            Detect();
        }

        void Save()
        {
            std::filesystem::create_directories(ProjectRoot / "Settings");
            std::ofstream(ProjectRoot / "Settings/ExternalTools.ini") << std::quoted(Executable);
        }

        void Detect()
        {
            if (Detection.valid())
            {
                return;
            }
            const auto root = ProjectRoot;
            Detection = std::async(std::launch::async, [root]
                                   {
                                       const auto* directory = std::getenv("ProgramFiles(x86)");
                                       const auto locator = std::filesystem::path(directory ? directory : "C:/Program Files (x86)") / "Microsoft Visual Studio/Installer/vswhere.exe";
                                       return Process::Run({ locator.wstring(), L"-all", L"-prerelease", L"-legacy", L"-products",
                                                             L"*", L"-sort", L"-property", L"installationPath" },
                                                           root);
                                   });
        }

        void Update()
        {
            const auto now = std::chrono::steady_clock::now();
            if (!PendingVisualStudioSource.empty() && now >= VisualStudioRetry)
            {
                VisualStudioRetry = now + std::chrono::milliseconds(500);
                if (OpenInRunningSolution(PendingVisualStudioSolution, PendingVisualStudioSource))
                {
                    PendingVisualStudioSource.clear();
                    Status = "Opened file in project solution";
                }
                else if (now >= VisualStudioDeadline)
                {
                    PendingVisualStudioSource.clear();
                    Status = "Solution opened; Visual Studio did not become ready to open the selected file";
                    Logger::GetClientLogger()->error("{}", Status);
                }
            }
            if (SolutionGeneration.valid() && SolutionGeneration.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                const auto result = SolutionGeneration.get();
                const auto source = std::exchange(PendingSource, {});
                if (result.ExitCode == 0)
                {
                    OpenScript(source, false);
                }
                else
                {
                    Status = result.Output;
                    Logger::GetClientLogger()->error("Solution generation failed: {}", Status);
                }
            }
            if (!Detection.valid() || Detection.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            {
                return;
            }
            const auto result = Detection.get();
            Installations.clear();
            std::istringstream lines(result.Output);
            std::string line;
            while (std::getline(lines, line))
            {
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                const auto executable = std::filesystem::path(line) / "Common7/IDE/devenv.exe";
                if (std::filesystem::is_regular_file(executable))
                {
                    Installations.push_back(executable.string());
                }
            }
            constexpr const char* environmentVariables[] =
            {
                "LOCALAPPDATA",
                "ProgramFiles",
                "ProgramFiles(x86)"
            };

            for (const auto* variable : environmentVariables)
            {
                if (const auto* base = std::getenv(variable))
                {
                    constexpr const char* relativeValues[] =
                    {
                        "Programs/Microsoft VS Code/Code.exe",
                        "Microsoft VS Code/Code.exe",
                        "Programs/Microsoft VS Code Insiders/Code - Insiders.exe",
                        "Microsoft VS Code Insiders/Code - Insiders.exe"
                    };

                    for (const auto& relative : relativeValues)
                    {
                        const auto executable = std::filesystem::path(base) / relative;
                        if (std::filesystem::is_regular_file(executable) && std::find(Installations.begin(), Installations.end(), executable.string()) == Installations.end())
                        {
                            Installations.push_back(executable.string());
                        }
                    }
                }
            }
            if (Executable.empty() && !Installations.empty())
            {
                Executable = Installations.front();
                Save();
            }
            Status = !Installations.empty() ? "External editor discovery complete"
                                            : "No editors detected; select an executable manually";
        }

        bool OpenScript(const std::filesystem::path& source, bool allowGeneration = true)
        {
            if (!std::filesystem::is_regular_file(source))
            {
                return false;
            }
            if (Executable.empty())
            {
                Status = "Select a script editor under Project Settings > External Tools";
                return false;
            }
            const std::filesystem::path executable(Executable);
            auto solution = ProjectRoot / (ProjectName + ".slnx");
            bool included = false;
            std::ifstream manifest(ProjectRoot / ".Finality/NativeScripts/IDEFiles.txt");
            const auto sourceKey = std::filesystem::absolute(source).lexically_normal().generic_string();
            std::string line;
            while (std::getline(manifest, line))
            {
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                std::error_code error;
                if (_stricmp(line.c_str(), sourceKey.c_str()) == 0 || std::filesystem::equivalent(std::filesystem::path(line), source, error))
                {
                    included = true;
                    break;
                }
            }
            if (!std::filesystem::is_regular_file(solution) && std::filesystem::is_regular_file(ProjectRoot / (ProjectName + ".sln")))
            {
                solution = ProjectRoot / (ProjectName + ".sln");
            }
            if (!included || !std::filesystem::is_regular_file(solution) || !std::filesystem::is_regular_file(ProjectRoot / "GameScripts.vcxproj") || !std::filesystem::is_regular_file(ProjectRoot / (ProjectName + ".code-workspace")))
            {
                if (!allowGeneration)
                {
                    Status = "The selected file could not be added to the project solution";
                    Logger::GetClientLogger()->error("{}: {}", Status, source.string());
                    return false;
                }
                if (SolutionGeneration.valid())
                {
                    return false;
                }
                auto engine = FindDevelopmentRoot(Process::ExecutablePath().parent_path());
                if (engine.empty())
                {
                    Status = "Native script tools were not found";
                    return false;
                }
                PendingSource = source;
                const auto root = ProjectRoot;
                const auto bin = Process::ExecutablePath().parent_path();
                const auto scripts = ScriptDirectory;
                const auto projectName = ProjectName;
                SolutionGeneration =
                    std::async(std::launch::async, [engine, root, bin, scripts, projectName]
                               {
#ifdef FINALITY_DIST
                                   const std::wstring configuration = L"Dist";
#elif defined(NDEBUG)
                                   const std::wstring configuration = L"Release";
#else
                                   const std::wstring configuration = L"Debug";
#endif
                                   return Process::Run({ (bin / "FIBuild.exe").wstring(), L"native",
                                                         L"--project",
                                                         root.wstring(),
                                                         L"--name",
                                                         std::filesystem::path(projectName).wstring(),
                                                         L"--engine",
                                                         engine.wstring(),
                                                         L"--bin",
                                                         bin.wstring(),
                                                         L"--source",
                                                         std::filesystem::path(scripts).wstring(),
                                                         L"--configuration",
                                                         configuration,
                                                         L"--generate-only" },
                                                       root);
                               });
                Status = "Generating engine-linked solution";
                return true;
            }
            if (_wcsicmp(executable.filename().c_str(), L"devenv.exe") == 0 && OpenInRunningSolution(solution, source))
            {
                Status = "Opened script in project solution";
                return true;
            }
            if (_wcsicmp(executable.filename().c_str(), L"devenv.exe") == 0 && !PendingVisualStudioSource.empty() && PendingVisualStudioSolution == solution)
            {
                PendingVisualStudioSource = source;
                return true;
            }
            const auto arguments = ScriptArguments(executable, ProjectRoot, solution, source);
            const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW( nullptr, L"open", executable.c_str(), arguments.c_str(), ProjectRoot.c_str(), SW_SHOWNORMAL));
            Status = result > 32 ? "Opened script in external editor"
                                 : "Could not launch the selected external editor";
            if (result <= 32)
            {
                Logger::GetClientLogger()->error("{}", Status);
            }
            else if (_wcsicmp(executable.filename().c_str(), L"devenv.exe") == 0)
            {
                PendingVisualStudioSource = source;
                PendingVisualStudioSolution = solution;
                VisualStudioDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
                VisualStudioRetry = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
                Status = "Opening project solution in Visual Studio";
            }
            return result > 32;
        }
    };
} // namespace FINALITY
