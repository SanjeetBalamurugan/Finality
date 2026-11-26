#include "EditorDependencies.h"
#include "DevelopmentSDK.h"
#include <Core/Process.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace
{
    namespace fs = std::filesystem;

    bool File(const fs::path& path)
    {
        std::error_code error;
        return fs::is_regular_file(path, error);
    }

    bool ValidAssets(const fs::path& path)
    {
        return File(path / "Editor/Shaders/editor_grid.vert") &&
               File(path / "Editor/Shaders/editor_grid.frag") &&
               File(path / "shaders/imported_model.vert") &&
               File(path / "shaders/imported_model.frag");
    }

    bool HasShaderCompiler(const fs::path& binaryDirectory)
    {
#ifdef _WIN32
        if (File(binaryDirectory / "glslc.exe"))
        {
            return true;
        }
        if (const auto* sdk = std::getenv("VULKAN_SDK"))
        {
            if (File(fs::path(sdk) / "Bin/glslc.exe"))
            {
                return true;
            }
        }
        return SearchPathW(nullptr, L"glslc.exe", nullptr, 0, nullptr, nullptr) != 0;
#else
        if (File(binaryDirectory / "glslc"))
        {
            return true;
        }
        if (const auto* paths = std::getenv("PATH"))
        {
            std::istringstream entries(paths);
            std::string directory;
            while (std::getline(entries, directory, ':'))
            {
                if (File(fs::path(directory) / "glslc"))
                {
                    return true;
                }
            }
        }
        return false;
#endif
    }

#ifdef _WIN32
    std::wstring Wide(const std::string& text)
    {
        const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        std::wstring result(count, L'\0');
        if (count)
        {
            MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), count);
        }
        return result;
    }
#endif
} // namespace

std::string FINALITY::EditorDependencyReport::Describe() const
{
    if (Errors.empty() && Warnings.empty())
    {
        return "Editor dependencies are ready.\n";
    }
    std::string text;
    if (!Errors.empty())
    {
        text += "The editor cannot start until these problems are fixed:\n\n";
        for (const auto& issue : Errors)
        {
            text += "- " + issue + "\n\n";
        }
    }
    if (!Warnings.empty())
    {
        text += "Some editor features are unavailable:\n\n";
        for (const auto& issue : Warnings)
        {
            text += "- " + issue + "\n\n";
        }
    }
    return text;
}

FINALITY::EditorDependencyReport FINALITY::EditorDependencies::Inspect(const fs::path& binaryDirectory)
{
    EditorDependencyReport report;
    fs::path engineRoot, bundledAssets;
    for (auto candidate = binaryDirectory; !candidate.empty(); candidate = candidate.parent_path())
    {
        if (engineRoot.empty() && File(candidate / "Scripts/Templates/ProjectScripts.lua"))
        {
            engineRoot = candidate;
        }
        if (report.Assets.empty() && ValidAssets(candidate / "FinalityEditor/assets"))
        {
            report.Assets = candidate / "FinalityEditor/assets";
        }
        if (bundledAssets.empty() && ValidAssets(candidate / "assets"))
        {
            bundledAssets = candidate / "assets";
        }
        if (candidate == candidate.parent_path())
        {
            break;
        }
    }
    if (report.Assets.empty())
    {
        report.Assets = bundledAssets;
    }
    engineRoot = FindDevelopmentRoot(binaryDirectory, report.Assets);
    if (report.Assets.empty())
    {
        report.Errors.push_back("Editor assets are missing or incomplete. Extract the complete editor package, including its assets folder, or rebuild FinalityEditor.");
    }
    if (!HasShaderCompiler(binaryDirectory))
    {
        report.Errors.push_back("Shader compiler glslc was not found. Restore glslc.exe beside FinalityEditor.exe, or install the Vulkan SDK and restart the editor. The SDK is not required when the bundled compiler is present.");
    }

    if (engineRoot.empty())
    {
        report.Warnings.push_back("Native scripting: the development SDK is missing. Restore the matching SDK folder beside the editor. You can continue editing without compiling native scripts.");
    }
    else if (!File(binaryDirectory / "FIBuild.exe"))
    {
        report.Warnings.push_back("Native scripting: FIBuild.exe is missing. Restore the complete editor development package or rebuild the solution.");
    }
    else
    {
#ifdef FINALITY_DIST
        const std::wstring configuration = L"Dist";
#elif defined(NDEBUG)
        const std::wstring configuration = L"Release";
#else
        const std::wstring configuration = L"Debug";
#endif
        const auto check = Process::Run({ (binaryDirectory / "FIBuild.exe").wstring(), L"check",
                                          L"--engine", engineRoot.wstring(), L"--bin", binaryDirectory.wstring(), L"--configuration", configuration },
                                        binaryDirectory);
        if (check.ExitCode != 0)
        {
            report.Warnings.push_back("Native scripting: " + check.Output);
        }
    }
    if (!File(binaryDirectory / "FIBaker.exe") || !File(binaryDirectory / "FinalityPlayer.exe"))
    {
        report.Warnings.push_back("Game export: FIBaker.exe or FinalityPlayer.exe is missing. Restore the complete editor package or build both targets in Visual Studio.");
    }
    return report;
}

bool FINALITY::EditorDependencies::ConfirmStartup(const fs::path& binaryDirectory, EditorDependencyReport& report)
{
    while (!report.Errors.empty() || !report.Warnings.empty())
    {
        const bool blocked = !report.Errors.empty();
        const auto message = report.Describe() + (blocked ? "Retry checks again after you repair the installation. Cancel exits." : "Continue opens the editor with these features unavailable. Try Again checks after repairs. Cancel exits.");
#ifdef _WIN32
        const int choice = MessageBoxW(nullptr, Wide(message).c_str(), L"Finality - Dependency Check",
                                       MB_SETFOREGROUND | (blocked ? MB_ICONERROR | MB_RETRYCANCEL : MB_ICONWARNING | MB_CANCELTRYCONTINUE));
        if (!blocked && choice == IDCONTINUE)
        {
            return true;
        }
        if (choice != IDRETRY && choice != IDTRYAGAIN)
        {
            return false;
        }
        report = Inspect(binaryDirectory);
#else
        std::cerr << message << '\n';
        return !blocked;
#endif
    }
    return true;
}

void FINALITY::EditorDependencies::ShowStartupError(const std::string& message)
{
#ifdef _WIN32
    MessageBoxW(nullptr, Wide(message).c_str(), L"Finality - Startup Failed", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#else
    std::cerr << message << '\n';
#endif
}
