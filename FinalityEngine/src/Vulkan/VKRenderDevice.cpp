#include "VKRenderDevice.h"

#include <vector>

#include <GLFW/glfw3.h>
#include "VkHelpers.h"
#include "VKDebug.h"

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
	//deviceFeatures.geometryShader = VK_TRUE; // TODO: Maybe in future, add error handling here, too lazy i am now
	//deviceFeatures.tessellationShader = VK_TRUE; // TODO: Same Here

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

void FINALITY::VKRenderDevice::CreateSwapChain()
{
	const VkSurfaceCapabilitiesKHR& SurfaceCaps = m_Devices.SelectedDevice().surfaceCapabilities;
	uint32_t NumImages = ChooseNumImages(SurfaceCaps);

	const std::vector<VkPresentModeKHR>& PresentModes = m_Devices.SelectedDevice().presentModes;
	VkPresentModeKHR PresentMode = ChoosePresentMode(PresentModes);

	VkSurfaceFormatKHR SurfaceFormat = ChooseSurfaceFormatAndColorSpace(m_Devices.SelectedDevice().surfaceFormats);
	m_SurfaceFormats = SurfaceFormat;

	VkSwapchainCreateInfoKHR SwapChainCreateInfo{};
	SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapChainCreateInfo.pNext = nullptr;
	SwapChainCreateInfo.flags = 0;
	SwapChainCreateInfo.surface = m_Surface;
	SwapChainCreateInfo.minImageCount = NumImages;
	SwapChainCreateInfo.imageFormat = SurfaceFormat.format;
	SwapChainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
	SwapChainCreateInfo.imageExtent = SurfaceCaps.currentExtent;
	SwapChainCreateInfo.imageArrayLayers = 1;
	SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	SwapChainCreateInfo.queueFamilyIndexCount = 1;
	SwapChainCreateInfo.pQueueFamilyIndices = &m_QueueFamily;
	SwapChainCreateInfo.preTransform = SurfaceCaps.currentTransform;
	SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapChainCreateInfo.presentMode = PresentMode;

	VkResult res = vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_SwapChain);
	CHECK_VK_RESULT(res, "vkCreateSwapchainKHR");

	uint32_t NumSwapChainImages = 0;
	res = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &NumSwapChainImages, nullptr);
	CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR");
	assert(NumImages == NumSwapChainImages);

	m_Images.resize(NumSwapChainImages);
	m_ImageViews.resize(NumSwapChainImages);

	res = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &NumSwapChainImages, m_Images.data());
	CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR");

	int LayerCount = 1;
	int MipLevels = 1;

	for (size_t i = 0; i < NumSwapChainImages; i++)
	{
		m_ImageViews[i] = CreateImageView(m_Device, m_Images[i], SurfaceFormat.format,
			VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, LayerCount, MipLevels);
	}
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

void FINALITY::VKRenderDevice::RecordCommandBuffers()
{
	VkClearValue ClearValue{};
	ClearValue.color = m_ClearColor;

	VkRenderPassBeginInfo RenderPassBeginInfo{};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassBeginInfo.pNext = nullptr;
	RenderPassBeginInfo.renderPass = m_RenderPass;
	RenderPassBeginInfo.renderArea.offset.x = 0;
	RenderPassBeginInfo.renderArea.offset.y = 0;
	RenderPassBeginInfo.renderArea.extent.width = m_Spec.width;
	RenderPassBeginInfo.renderArea.extent.height = m_Spec.height;
	RenderPassBeginInfo.clearValueCount = 1;
	RenderPassBeginInfo.pClearValues = &ClearValue;

	for (uint32_t i = 0; i < m_CMDBuffers.size(); i++) {
		vkResetCommandBuffer(m_CMDBuffers[i], 0);
		
		this->BeginCommandBuffers(m_CMDBuffers[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		RenderPassBeginInfo.framebuffer = m_FrameBuffers[i];
		vkCmdBeginRenderPass(m_CMDBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdEndRenderPass(m_CMDBuffers[i]);

		VkResult res = vkEndCommandBuffer(m_CMDBuffers[i]);
		CHECK_VK_RESULT(res, "vkEndCommandBuffer error");
	}
}

std::vector<VkFramebuffer> FINALITY::VKRenderDevice::CreateFrameBuffers() const
{
	std::vector<VkFramebuffer> FrameBuffers;
	FrameBuffers.resize(m_Images.size());

	for (uint32_t i = 0; i < m_Images.size(); i++) {
		std::vector<VkImageView> Attachments;
		Attachments.push_back(m_ImageViews[i]);

		VkFramebufferCreateInfo fbCreateInfo{};
		fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCreateInfo.pNext = nullptr;
		fbCreateInfo.renderPass = m_RenderPass;
		fbCreateInfo.attachmentCount = (uint32_t)Attachments.size();
		fbCreateInfo.pAttachments = Attachments.data();
		fbCreateInfo.width = (uint32_t)m_Spec.width;
		fbCreateInfo.height = (uint32_t)m_Spec.height;
		fbCreateInfo.layers = 1;

		VkResult res = vkCreateFramebuffer(m_Device, &fbCreateInfo, NULL, &FrameBuffers[i]);
		CHECK_VK_RESULT(res, "vkCreateFramebuffer");
	}
	return FrameBuffers;
}

VkRenderPass FINALITY::VKRenderDevice::CreateSimpleRenderPass() const
{
	VkAttachmentDescription ColorAttachment{};
	ColorAttachment.flags = 0;
	ColorAttachment.format = m_SurfaceFormats.format;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference ColorAttachRef{};
	ColorAttachRef.attachment = 0;
	ColorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription SubpassDesc{};
	SubpassDesc.flags = 0;
	SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDesc.inputAttachmentCount = 0;
	SubpassDesc.pInputAttachments = nullptr;
	SubpassDesc.colorAttachmentCount = 1;
	SubpassDesc.pColorAttachments = &ColorAttachRef;
	SubpassDesc.pResolveAttachments = nullptr;
	SubpassDesc.pDepthStencilAttachment = nullptr;
	SubpassDesc.preserveAttachmentCount = 0;
	SubpassDesc.pPreserveAttachments = nullptr;

	std::vector<VkAttachmentDescription> Attachments;
	Attachments.push_back(ColorAttachment);

	VkRenderPassCreateInfo RenderPassCreateInfo{};
	RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	RenderPassCreateInfo.pNext = nullptr;
	RenderPassCreateInfo.flags = 0;
	RenderPassCreateInfo.attachmentCount = (uint32_t)Attachments.size();
	RenderPassCreateInfo.pAttachments = Attachments.data();
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses = &SubpassDesc;
	RenderPassCreateInfo.dependencyCount = 0;
	RenderPassCreateInfo.pDependencies = nullptr;

	VkRenderPass RenderPass;
	VkResult res = vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &RenderPass);
	CHECK_VK_RESULT(res, "vkCreateRenderPass");

	return RenderPass;
}

void FINALITY::VKRenderDevice::DestroyFramebuffers()
{
	for (int i = 0; i < m_FrameBuffers.size(); i++) {
		vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
	}
}

void FINALITY::VKRenderDevice::Initialize(const NativeWindowHandle& handle)
{
	this->CreateInstance();
	this->SetupDebugMessanger();
	this->CreateSurface(handle);

	m_Devices.Initialize(m_Instance, m_Surface);
	m_QueueFamily = m_Devices.SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);

	this->CreateDevice();
	this->CreateSwapChain();
	this->CreateCommandBufferPool();

	m_Queue.Initialize(m_Device, m_SwapChain, m_QueueFamily, 0);
	m_RenderPass = this->CreateSimpleRenderPass();
	m_FrameBuffers = this->CreateFrameBuffers();
	this->CreateCommandBuffers(m_Images.size());
}

void FINALITY::VKRenderDevice::Shutdown()
{
	m_Queue.ShutDown();
	this->DestroyFramebuffers();
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	if (m_CMDBufPool) vkDestroyCommandPool(m_Device, m_CMDBufPool, nullptr);

	for (uint32_t i = 0; i < m_ImageViews.size(); i++)
	{
		vkDestroyImageView(m_Device, m_ImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);

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
}

void FINALITY::VKRenderDevice::EndFrame()
{
	this->RecordCommandBuffers();
	m_Queue.SubmitASync(m_CMDBuffers[m_ImageIndex]);
}

void FINALITY::VKRenderDevice::PresentFrame()
{
	m_Queue.Present(m_ImageIndex);
	m_Queue.WaitIdle(); // Temp fix for now
}

void FINALITY::VKRenderDevice::Clear(float r, float g, float b, float a)
{
	m_ClearColor = { r, g, b, a };
}
