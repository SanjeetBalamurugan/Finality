/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKShader.h.
 */
#pragma once
#include "VKCore.h"
#include <Renderer/Shader.h>
#include <string>

namespace FINALITY
{
    /** @todo TODO: Document VKShader class semantics, ownership and usage. */
    class VKShader : public Shader
    {
    public:
        VKShader(VkDevice device, const std::string& filePath);
        ~VKShader() override;

        void Load(const std::string& filePath) override;

        RendererAPI GetAPI() const override
        {
            return RendererAPI::VULKAN;
        }

        VkShaderModule GetVkModule() const
        {
            return m_ShaderModule;
        }

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
    };
} // namespace FINALITY
