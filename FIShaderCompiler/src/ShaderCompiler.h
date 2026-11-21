#pragma once

#include <string>

/** @todo TODO: Document ShaderCompiler class semantics, ownership and usage. */
class ShaderCompiler
{
public:
    virtual ~ShaderCompiler() = default;

    virtual bool Compile(
        const std::string& inputFile,
        const std::string& outputFile
    ) = 0;
};