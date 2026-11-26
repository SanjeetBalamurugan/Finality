#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace FINALITY
{
    /** @brief Startup prerequisite results; optional tool failures do not prevent basic editing. */
    struct EditorDependencyReport
    {
        std::filesystem::path Assets;
        std::vector<std::string> Errors;
        std::vector<std::string> Warnings;
        std::string Describe() const;
    };

    /** @brief Check installation files before graphics initialization and explain repairs in a native dialog. */
    class EditorDependencies
    {
    public:
        static EditorDependencyReport Inspect(const std::filesystem::path& binaryDirectory);
        static bool ConfirmStartup(const std::filesystem::path& binaryDirectory, EditorDependencyReport& report);
        static void ShowStartupError(const std::string& message);
    };
} // namespace FINALITY
