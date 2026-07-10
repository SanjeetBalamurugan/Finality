#define ARGS_COUNT 4

#include "SpirvCompiler.h"

#include <iostream>

int main(int argc, char const* argv[])
{
    if (argc < ARGS_COUNT)
    {
        std::cout
            << "USAGE: FIShaderCompiler <inputFile> <outputFile> --[vulkan|]"
            << std::endl;

        return EXIT_FAILURE;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    std::string graphicsAPI = argv[3];

    if (graphicsAPI == "--vulkan")
    {
        SpirvCompiler compiler;

        if (!compiler.Compile(inputFile, outputFile))
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}