/**
 * @file
 * @ingroup core
 * @brief Runs external tools with captured diagnostics.
 *
 * ProcessResult contains the exit code and captured output. The argument-vector interface keeps file paths and tool options separate, including paths with spaces.
 *
 * @par Usage
 * Run long-lived compilation work on a background task. Check ExitCode before using produced files and present Output when the tool reports a failure.
 */
#pragma once

#include "Core.h"
#include <filesystem>
#include <string>
#include <vector>

namespace FINALITY
{
    /** @brief Exit status and captured diagnostic output from an external tool invocation. */
    struct ProcessResult
    {
        int ExitCode = -1;
        std::string Output;
    };

    /**
     * @ingroup core
     * @brief Runs external tools with captured diagnostics.
     *
     * ProcessResult contains the exit code and captured output. The argument-vector interface keeps file paths and tool options separate, including paths with spaces.
     *
     * @par Usage
     * Run long-lived compilation work on a background task. Check ExitCode before using produced files and present Output when the tool reports a failure.
     */
    class FAPI Process
    {
    public:
        static ProcessResult Run(const std::vector<std::wstring>& arguments, const std::filesystem::path& directory);
        static std::filesystem::path ExecutablePath();
    };
}
