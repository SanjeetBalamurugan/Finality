#define ARGS_COUNT 4

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#include <shaderc/shaderc.hpp>

int main(int argc, char const *argv[])
{
    if (argc < ARGS_COUNT)
    {
        std::cout << "USAGE: FIShaderCompiler <inputFile> <outputFile> --[vulkan|]" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::filesystem::path inputFilePath = argv[1];
    std::string outputFilePath = argv[2];
    std::string graphicsAPI = argv[3];

    std::cout << inputFilePath << std::endl;

    std::ifstream inputFile(inputFilePath);
    if (!inputFile.is_open())
    {
        std::cerr << "Failed to open File at " << inputFilePath << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::string shaderSource((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());

    shaderc::Compiler compiler;
    shaderc::CompileOptions compilerOptions;
    compilerOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc_shader_kind kind = shaderc_glsl_vertex_shader;
    if (inputFilePath.extension() == ".frag") {
        kind = shaderc_glsl_fragment_shader;
    }

    std::string inputFileName = inputFilePath.filename().string();
    auto result = compiler.CompileGlslToSpv(shaderSource, kind, inputFileName.c_str(), compilerOptions);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Compilation failed: " << result.GetErrorMessage() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::ofstream outFile(outputFilePath, std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(result.cbegin()), (result.cend() - result.cbegin()) * sizeof(uint32_t));

    std::cin.get();
    std::exit(EXIT_SUCCESS);
}
