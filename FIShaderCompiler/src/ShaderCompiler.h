#pragma once

#include <string>

class ShaderCompiler
{
public:
    virtual ~ShaderCompiler() = default;

    virtual bool Compile(
        const std::string& inputFile,
        const std::string& outputFile
    ) = 0;
};