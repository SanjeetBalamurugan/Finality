#include "Texture.h"
#include <Vulkan/VKRenderDevice.h>
#include "Application.h"
#include <Vulkan/VKTexture.h>

std::shared_ptr<FINALITY::Texture> FINALITY::Texture::Create(const std::string& filePath)
{
    switch (FINALITY::Application::GetActiveAPI())
    {
    case RendererAPI::NONE:
    {
        throw std::runtime_error("Renderer API is currently set to None!");
        return nullptr;
    }
    case RendererAPI::VULKAN:
    {
        auto* renderDevice = static_cast<VKRenderDevice*>(&FINALITY::Application::Get().GetDevice());
        if (!renderDevice)
        {
            throw std::runtime_error("Vulkan Render Device is invalid or uninitialized!");
        }

        return std::make_shared<VKTexture>(
            renderDevice->GetActiveDevice(),
            renderDevice->GetActivePhysicalDevice(),
            renderDevice->GetCommandPool(),
            renderDevice->GetGraphicsQueue(),
            filePath
        );
    }
    default:
    {
        throw std::runtime_error("Unknown or unsupported graphics API configuration layer!");
        return nullptr;
    }
    }
}
