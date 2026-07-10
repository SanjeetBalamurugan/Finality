#include "SpirvCompiler.h"

#include <iostream>
#include <fstream>
#include <filesystem>

bool SpirvCompiler::Compile(
    const std::string& inputFile,
    const std::string& outputFile
)
{
    std::filesystem::path inputPath(inputFile);

    std::ifstream file(inputPath);

    if (!file.is_open())
    {
        std::cerr << "Failed to open File at " << inputPath << std::endl;
        return false;
    }

    std::string shaderSource(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    m_Options.SetOptimizationLevel(
        shaderc_optimization_level_performance
    );

    shaderc_shader_kind kind = GetShaderKind(inputFile);

    std::string inputFileName = inputPath.filename().string();

    auto result = m_Compiler.CompileGlslToSpv(
        shaderSource,
        kind,
        inputFileName.c_str(),
        m_Options
    );

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << "Compilation failed:\n"
            << result.GetErrorMessage()
            << std::endl;

        return false;
    }

    std::ofstream outFile(outputFile, std::ios::binary);

    if (!outFile.is_open())
    {
        std::cerr << "Failed to open output file at "
            << outputFile
            << std::endl;

        return false;
    }

    outFile.write(
        reinterpret_cast<const char*>(result.cbegin()),
        static_cast<std::streamsize>(
            (result.cend() - result.cbegin()) * sizeof(uint32_t)
            )
    );

    outFile.close();

    return true;
}


shaderc_shader_kind SpirvCompiler::GetShaderKind(const std::string& file)
{
    std::filesystem::path path(file);

    if (path.extension() == ".frag")
        return shaderc_glsl_fragment_shader;

    return shaderc_glsl_vertex_shader;
}