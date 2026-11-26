#include "Shader.h"
#include <stdexcept>

std::unique_ptr<FINALITY::Shader> FINALITY::Shader::Create(const std::string& path, RenderDevice* device)
{
    if (!device)
    {
        throw std::invalid_argument("Shader creation requires a render device");
    }
    return device->CreateShader(path);
}
