#include <Vulkan/VKCore.h>

#include "VKImGUIRenderer.h"
#include <Vulkan/VKRenderDevice.h>
#include <Vulkan/VkHelpers.h>
#include <backends/imgui_impl_glfw.h>

static void CHECK_VK_RESULT_IMGUI(VkResult err)
{
	if (err == 0) return;

	CHECK_VK_RESULT(err, "ImGUI Vulkan:");
	if (err < 0) abort();
}

void FINALITY::VKImGUIRenderer::Initialize()
{
	this->CreateDescriptorPool();
	this->InitImGUI();

	VKRenderDevice* vkDevice = static_cast<VKRenderDevice*>(this->GetRenderDevice());
	VkFormat ColorFormat = vkDevice->GetSwapChain()->GetVKFormat();

	VkPipelineRenderingCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	createInfo.pNext = VK_NULL_HANDLE;
	createInfo.viewMask = 0;
	createInfo.colorAttachmentCount = 1;
	createInfo.pColorAttachmentFormats = &ColorFormat;
	createInfo.depthAttachmentFormat = vkDevice->GetDepthFormat();
	createInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	ImGui_ImplVulkan_PipelineInfo pInfo{};
	pInfo.Subpass = 0;
	pInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	pInfo.PipelineRenderingCreateInfo = createInfo;
	pInfo.RenderPass = VK_NULL_HANDLE;

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.ApiVersion = vkDevice->GetAppInfo().apiVersion;
	initInfo.Instance = vkDevice->GetInstance();
	initInfo.PhysicalDevice = vkDevice->GetActivePhysicalDevice();
	initInfo.Device = vkDevice->GetActiveDevice();
	initInfo.QueueFamily = vkDevice->GetQueueFamily();
	initInfo.Queue = vkDevice->GetGraphicsQueue();
	initInfo.DescriptorPool = m_DescriptorPool;
	initInfo.MinImageCount = vkDevice->GetSurfaceCaps().minImageCount;
	initInfo.ImageCount = (uint32_t)vkDevice->GetNumImages();
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.UseDynamicRendering = true;
	initInfo.PipelineInfoMain = pInfo;
	initInfo.Allocator = VK_NULL_HANDLE;
	initInfo.CheckVkResultFn = CHECK_VK_RESULT_IMGUI;

	ImGui_ImplVulkan_Init(&initInfo);

	m_CMDBufs.resize(vkDevice->GetNumImages());
	vkDevice->CreateCommandBuffers(vkDevice->GetNumImages(), m_CMDBufs.data());
}

VkCommandBuffer FINALITY::VKImGUIRenderer::PrepareCommandBuffer(int image)
{
	VKRenderDevice* vkDevice = static_cast<VKRenderDevice*>(this->GetRenderDevice());
	vkDevice->BeginCommandBuffers(m_CMDBufs[image], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	ImageMemBarrier(m_CMDBufs[image], vkDevice->GetImages(image),
		vkDevice->GetSwapChainFormat(),
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1, 0);

	vkDevice->BeginDynamicRendering(m_CMDBufs[image], image, NULL, NULL);

	ImDrawData* pDrawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(pDrawData, m_CMDBufs[image]);

	pfnCmdEndRenderingKHR(m_CMDBufs[image]);

	ImageMemBarrier(m_CMDBufs[image], vkDevice->GetImages(image),
		vkDevice->GetSwapChainFormat(),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, 1, 0);

	vkEndCommandBuffer(m_CMDBufs[image]);

	return m_CMDBufs[image];
}

void FINALITY::VKImGUIRenderer::Destroy()
{
	VKRenderDevice* vkDevice = static_cast<VKRenderDevice*>(this->GetRenderDevice());
	vkDevice->FreeCommandBuffers((uint32_t)m_CMDBufs.size(), m_CMDBufs.data());

	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(vkDevice->GetActiveDevice(), m_DescriptorPool, nullptr);

	this->ShutDown();
}

void FINALITY::VKImGUIRenderer::Update()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
}

void FINALITY::VKImGUIRenderer::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pCreateInfo{};
	pCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pCreateInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
	pCreateInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
	pCreateInfo.pPoolSizes = poolSizes;

	VKRenderDevice* vkDevice = static_cast<VKRenderDevice*>(this->GetRenderDevice());
	VkResult res = vkCreateDescriptorPool(vkDevice->GetActiveDevice(), &pCreateInfo, nullptr, &m_DescriptorPool);
	CHECK_VK_RESULT(res, "ImGUI: vkCreateDescriptorPool")
}
