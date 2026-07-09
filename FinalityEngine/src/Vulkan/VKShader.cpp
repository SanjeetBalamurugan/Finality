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
	if(m_ShaderModule) vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
}

void FINALITY::VKShader::Load(const std::string& filePath)
{
	std::filesystem::path path(filePath); // expecting file like "./test.vert" and import "./test.vert.vk.spv"
	std::string extension = path.extension().string();
	extension += ".vk.spv"; // Expected the Compiled binary file name to be in this format
	path.replace_extension(extension);

	std::ifstream file(path.string(), std::ios::ate | std::ios::binary);
	if (!file.is_open())
		FI_CORE_ERROR("Failed to load VULKAN Shader Binary at {}", path.string());

	uint32_t fileSize = file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = buffer.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
	
	VkResult res = vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_ShaderModule);
	CHECK_VK_RESULT(res, "vkCreateShaderModule error");
}
