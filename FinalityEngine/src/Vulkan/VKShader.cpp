#include "VKShader.h"

#include <fstream>
#include <filesystem>
#include <Renderer/ShaderCompiler.h>
#include <stdexcept>

FINALITY::VKShader::VKShader(VkDevice device, const std::string& filePath)
	: m_Device(device)
{
	this->Load(filePath);
}

FINALITY::VKShader::~VKShader()
{
	if (m_ShaderModule)
	{
		vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
		m_ShaderModule = VK_NULL_HANDLE;
	}
}

void FINALITY::VKShader::Load(const std::string& filePath)
{
    if (!ShaderCompiler::EnsureCompiled(filePath))
        throw std::runtime_error("Shader compilation failed: " + filePath);
    std::filesystem::path path(filePath);
    path += ".spv";

	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		FI_CORE_ERROR("Failed to load VULKAN Shader Binary at {}", path.string());
        throw std::runtime_error("Failed to read shader binary: " + path.string());
    }

	size_t fileSize = static_cast<size_t>(file.tellg());

	if (fileSize % sizeof(uint32_t) != 0)
	{
		FI_CORE_ERROR("Invalid SPIR-V file size: {}", fileSize);
        throw std::runtime_error("Invalid SPIR-V size: " + path.string());
    }

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    if (!file) throw std::runtime_error("Cannot read complete shader binary: " + path.string());
	file.close();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode = buffer.data();

    VkShaderModule candidate = VK_NULL_HANDLE;
    VkResult res = vkCreateShaderModule(m_Device, &createInfo, nullptr, &candidate);
    if (res != VK_SUCCESS)
        throw std::runtime_error("vkCreateShaderModule failed: " + path.string());
    if (m_ShaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
    m_ShaderModule = candidate;
}