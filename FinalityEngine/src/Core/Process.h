#pragma once

#include "Core.h"
#include <filesystem>
#include <string>
#include <vector>

namespace FINALITY
{
    struct ProcessResult
    {
        int ExitCode = -1;
        std::string Output;
    };

    class FAPI Process
    {
    public:
        static ProcessResult Run(const std::vector<std::wstring>& arguments, const std::filesystem::path& directory);
        static std::filesystem::path ExecutablePath();
    };
}
