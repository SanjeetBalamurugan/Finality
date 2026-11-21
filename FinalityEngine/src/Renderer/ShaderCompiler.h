/**
 * @file
 * @ingroup render
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ShaderCompiler.h.
 */
#pragma once
#include <Core/Core.h>
#include <string>

namespace FINALITY
{
    /** @todo TODO: Document ShaderCompiler class semantics, ownership and usage. */
    class FAPI ShaderCompiler
    {
    public:
        static bool EnsureCompiled(const std::string& sourcePath, bool force = false);
        static bool IsSourceShader(const std::string& path);
    };
}
