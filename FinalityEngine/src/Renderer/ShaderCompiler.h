#pragma once
#include <Core/Core.h>
#include <string>

namespace FINALITY
{
    class FAPI ShaderCompiler
    {
    public:
        static bool EnsureCompiled(const std::string& sourcePath, bool force = false);
        static bool IsSourceShader(const std::string& path);
    };
}
