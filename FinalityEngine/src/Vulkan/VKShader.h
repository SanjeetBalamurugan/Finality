#pragma once
#include "VKCore.h"

#include <Core/Shader.h>

namespace FINALITY
{
    class VKShader : public Shader
    {
    public:
        VKShader(VkDevice device, const std::string& filePath);
        ~VKShader() override;

        void Load(const std::string& filePath) override;
        RendererAPI GetAPI() const override { return RendererAPI::VULKAN; }

        VkShaderModule GetVkModule() const { return m_ShaderModule; }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
    };
}