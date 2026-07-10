#include "VKShader.h"

#include <fstream>
#include <filesystem>

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
	std::filesystem::path path(filePath); // expecting file like "./test.vert" and import "./test.vert.vk.spv"
	path += ".spv"; // Expected the Compiled binary file name to be in this format

	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		FI_CORE_ERROR("Failed to load VULKAN Shader Binary at {}", path.string());
		return;
	}

	size_t fileSize = static_cast<size_t>(file.tellg());

	if (fileSize % sizeof(uint32_t) != 0)
	{
		FI_CORE_ERROR("Invalid SPIR-V file size: {}", fileSize);
		return;
	}

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode = buffer.data();

	VkResult res = vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_ShaderModule);
	CHECK_VK_RESULT(res, "vkCreateShaderModule error");
}