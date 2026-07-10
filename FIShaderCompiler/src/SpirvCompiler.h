#pragma once

#include "ShaderCompiler.h"

#include <shaderc/shaderc.hpp>

class SpirvCompiler : public ShaderCompiler
{
public:
    SpirvCompiler() = default;
    ~SpirvCompiler() override = default;

    bool Compile(
        const std::string& inputFile,
        const std::string& outputFile
    ) override;

private:
    shaderc_shader_kind GetShaderKind(const std::string& file);

private:
    shaderc::Compiler m_Compiler;
    shaderc::CompileOptions m_Options;
};