#include "Shader.h"
#include "Application.h"

#include <Vulkan/VKRenderDevice.h>
#include <Vulkan/VKShader.h>

std::unique_ptr<FINALITY::Shader> FINALITY::Shader::Create(RenderDevice* device, const std::string& filePath)
{
    switch (FINALITY::Application::GetActiveAPI())
    {
    case RendererAPI::VULKAN:
    {
        auto* vkDevice = static_cast<VKRenderDevice*>(device);
        return std::make_unique<VKShader>(vkDevice->GetActiveDevice(), filePath);
    }
    default:
    {
        FI_CORE_ERROR("Invalid Renderer");
        assert(0);
        return nullptr;
    }
    }
}
