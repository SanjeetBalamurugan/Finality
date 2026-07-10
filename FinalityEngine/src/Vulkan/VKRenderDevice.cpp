#include "VKRenderDevice.h"
#include <vector>
#include <GLFW/glfw3.h>
#include "VkHelpers.h"
#include "VKDebug.h"
#include "VKMesh.h"
#include "VKPipeline.h"

void FINALITY::VKRenderDevice::CreateInstance()
{
	m_EnableValidationLayers = FINALITY::VKDebug::EnableValidationLayers;

	if (m_EnableValidationLayers && !VKDebug::CheckValidationLayerSupport())
	{
		FI_CORE_ERROR("validation layers requested, but not available!");
	}

	m_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	m_AppInfo.pApplicationName = m_Spec.title.c_str();
	m_AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	m_AppInfo.pEngineName = "FinalityEngine";
	m_AppInfo.engineVersion = VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);
	m_AppInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &m_AppInfo;

	auto extensions = VKDebug::GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (m_EnableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		VKDebug::PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
	CHECK_VK_RESULT(result, "Create Instance");
	FI_CORE_INFO("VULKAN instance created successfully");
}

void FINALITY::VKRenderDevice::SetupDebugMessanger()
{
	if (!m_EnableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	VKDebug::PopulateDebugMessengerCreateInfo(createInfo);

	VkResult res = VKDebug::CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessanger);
	CHECK_VK_RESULT(res, "Create Debug Messanger");
}

void FINALITY::VKRenderDevice::CreateSurface(const NativeWindowHandle& handle)
{
	GLFWwindow* window = (GLFWwindow*)handle.WindowHandle;
	VkResult res = glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface);
	CHECK_VK_RESULT(res, "Vulkan Surface Creation");
}

void FINALITY::VKRenderDevice::DestroySurface() const
{
	PFN_vkDestroySurfaceKHR vkDestroySurface = nullptr;
	vkDestroySurface = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(m_Instance, "vkDestroySurfaceKHR");

	if (!vkDestroySurface)
	{
		FI_CORE_ERROR("Cannot find the address of vkDestroySurfaceKHR");
	}

	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
}

void FINALITY::VKRenderDevice::CreateDevice()
{
	float qPriorities[] = { 1.0f };

	VkDeviceQueueCreateInfo qInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = m_QueueFamily,
		.queueCount = 1,
		.pQueuePriorities = &qPriorities[0]
	};

	std::vector<const char*> DevExts = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &qInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = (uint32_t)DevExts.size();
	createInfo.ppEnabledExtensionNames = DevExts.data();

	VkResult res = vkCreateDevice(m_Devices.SelectedDevice().device, &createInfo, nullptr, &m_Device);
	CHECK_VK_RESULT(res, "vkCreateDevice");
}

void FINALITY::VKRenderDevice::DestroyDevice()
{
	vkDestroyDevice(m_Device, nullptr);
}

void FINALITY::VKRenderDevice::CreateCommandBuffers(uint32_t count)
{
	m_CMDBuffers.resize(count);

	VkCommandBufferAllocateInfo cmdBufAllocInfo{};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.pNext = nullptr;
	cmdBufAllocInfo.commandPool = m_CMDBufPool;
	cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocInfo.commandBufferCount = count;

	VkResult res = vkAllocateCommandBuffers(m_Device, &cmdBufAllocInfo, m_CMDBuffers.data());
	CHECK_VK_RESULT(res, "vkAllocateCommandBuffers error");
}

void FINALITY::VKRenderDevice::CreateCommandBufferPool()
{
	VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cmdPoolCreateInfo.pNext = nullptr;
	cmdPoolCreateInfo.queueFamilyIndex = m_QueueFamily;

	VkResult res = vkCreateCommandPool(m_Device, &cmdPoolCreateInfo, nullptr, &m_CMDBufPool);
	CHECK_VK_RESULT(res, "vkCreateCommandPool error");
}

void FINALITY::VKRenderDevice::BeginCommandBuffers(VkCommandBuffer cmdBuf, uint32_t usageFlags)
{
	VkCommandBufferBeginInfo cmdBufBeginInfo{};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.pNext = nullptr;
	cmdBufBeginInfo.flags = usageFlags;
	cmdBufBeginInfo.pInheritanceInfo = nullptr;

	VkResult res = vkBeginCommandBuffer(cmdBuf, &cmdBufBeginInfo);
	CHECK_VK_RESULT(res, "vkBeginCommandBuffer error");
}

void FINALITY::VKRenderDevice::Initialize(const NativeWindowHandle& handle)
{
	this->CreateInstance();
	this->SetupDebugMessanger();
	this->CreateSurface(handle);

	m_Devices.Initialize(m_Instance, m_Surface);
	m_QueueFamily = m_Devices.SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);

	this->CreateDevice();

	m_SwapChain = std::make_unique<VKSwapChain>(m_Device, m_Devices.SelectedDevice().device, m_Surface);
	m_SwapChain->Initialize(handle, m_Spec);

	this->CreateCommandBufferPool();

	m_Queue.Initialize(m_Device, m_SwapChain->GetVKHandle(), m_QueueFamily, 0);
	this->CreateCommandBuffers(m_SwapChain->GetImageCount());
}

void FINALITY::VKRenderDevice::SetWindowSpec(const WindowSpec& spec)
{
	m_Spec = spec;
	if (m_SwapChain)
	{
		m_SwapChain->Recreate(m_Spec);
		m_Queue.UpdateSwapChain(m_SwapChain->GetVKHandle());
	}
}

void FINALITY::VKRenderDevice::DrawQueue(const std::vector<RenderPacket>& queue)
{
	VkCommandBuffer cmd = m_CMDBuffers[m_ImageIndex];

	VkViewport viewport{
		.x = 0.0f,
		.y = static_cast<float>(m_SwapChain->GetHeight()),
		.width = static_cast<float>(m_SwapChain->GetWidth()),
		.height = -static_cast<float>(m_SwapChain->GetHeight()),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = { m_SwapChain->GetWidth(), m_SwapChain->GetHeight() }
	};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	VKPipeline* activePipeline = nullptr;

	for (const auto& packet : queue)
	{
		if (!packet.MeshData || !packet.PipelineInstance) continue;

		auto* vkPipeline = static_cast<VKPipeline*>(packet.PipelineInstance.get());
		auto* vkMesh = static_cast<VKMesh*>(packet.MeshData.get());

		if (vkPipeline != activePipeline)
		{
			activePipeline = vkPipeline;
			activePipeline->Bind(cmd);
		}

		vkMesh->Bind(cmd);

		if (vkMesh->HasIndices())
		{
			vkCmdDrawIndexed(cmd, vkMesh->GetIndexCount(), 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(cmd, vkMesh->GetVertexCount(), 1, 0, 0);
		}
	}
}

std::shared_ptr<FINALITY::Mesh> FINALITY::VKRenderDevice::CreateMesh(const std::vector<Vertex>& vertices)
{
	return std::make_shared<VKMesh>(m_Device, m_Devices.SelectedDevice().device, vertices);
}

std::shared_ptr<FINALITY::Mesh> FINALITY::VKRenderDevice::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	return std::make_shared<VKMesh>(m_Device, m_Devices.SelectedDevice().device, vertices, indices);
}

std::shared_ptr<FINALITY::Pipeline> FINALITY::VKRenderDevice::CreatePipeline(const PipelineConfig& config)
{
	return std::make_shared<VKPipeline>(m_Device, m_SwapChain->GetVKRenderPass(), config);
}

void FINALITY::VKRenderDevice::Shutdown()
{
	m_Queue.WaitIdle();

	if (m_CMDBufPool) vkDestroyCommandPool(m_Device, m_CMDBufPool, nullptr);

	m_Queue.ShutDown();
	m_SwapChain->Shutdown();

	DestroyDevice();

	if (m_EnableValidationLayers) {
		VKDebug::DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessanger, nullptr);
	}

	DestroySurface();
	vkDestroyInstance(m_Instance, nullptr);
}

void FINALITY::VKRenderDevice::BeginFrame()
{
	m_ImageIndex = m_Queue.AcquireNextImage();

	if (m_ImageIndex == UINT32_MAX)
	{
		m_SwapChain->Recreate(m_Spec);
		m_Queue.UpdateSwapChain(m_SwapChain->GetVKHandle());
		m_ImageIndex = m_Queue.AcquireNextImage();
	}

	BeginCommandBuffers(m_CMDBuffers[m_ImageIndex], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	std::vector<VkClearValue> clearValues(2);
	clearValues[0].color = m_ClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo RenderPassBeginInfo{};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassBeginInfo.pNext = nullptr;
	RenderPassBeginInfo.renderPass = m_SwapChain->GetVKRenderPass();
	RenderPassBeginInfo.framebuffer = m_SwapChain->GetVKFramebuffer(m_ImageIndex);
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };
	RenderPassBeginInfo.renderArea.extent.width = m_SwapChain->GetWidth();
	RenderPassBeginInfo.renderArea.extent.height = m_SwapChain->GetHeight();

	RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	RenderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_CMDBuffers[m_ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void FINALITY::VKRenderDevice::EndFrame()
{
	vkCmdEndRenderPass(m_CMDBuffers[m_ImageIndex]);
	VkResult res = vkEndCommandBuffer(m_CMDBuffers[m_ImageIndex]);
	CHECK_VK_RESULT(res, "vkEndCommandBuffer error");

	m_Queue.SubmitASync(m_CMDBuffers[m_ImageIndex], m_ImageIndex);
}

void FINALITY::VKRenderDevice::PresentFrame()
{
	m_Queue.Present(m_ImageIndex);
}

void FINALITY::VKRenderDevice::Clear(float r, float g, float b, float a)
{
	m_ClearColor = { .float32 = { r, g, b, a } };
}
