#define ARGS_COUNT 4

#include <iostream>
#include <string>

int main(int argc, char const *argv[])
{
    if (argc < ARGS_COUNT)
    {
        std::cout << "USAGE: FIShaderCompiler <inputFile> <outputFile> --[vulkan|]" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << argv[1] << argv[2] << argv[3] << std::endl;

    std::cin.get();
    std::exit(EXIT_SUCCESS);
}
