#include "VKRenderDevice.h"

#include <vector>

#include <GLFW/glfw3.h>
#include "VkHelpers.h"

void FINALITY::VKRenderDevice::CreateInstance()
{
#ifdef NDEBUG
	m_EnableValidationLayers = false;
#else
	m_EnableValidationLayers = true;
#endif

	if (m_EnableValidationLayers && !CheckValidationLayerSupport())
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
	
	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (m_EnableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
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

bool FINALITY::VKRenderDevice::CheckValidationLayerSupport() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_ValidationLayers)
	{
		bool found = false;

		for (const auto& properties : availableLayers)
		{
			if (strcmp(layerName, properties.layerName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> FINALITY::VKRenderDevice::GetRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (m_EnableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL FINALITY::VKRenderDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		FI_CORE_ERROR("Validation [ERROR]: {}", pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		FI_CORE_WARN("Validation [WARNING]: {}", pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		FI_CORE_INFO("Validation [INFO]: {}", pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{
		FI_CORE_INFO("Validation [VERBOSE]: {}", pCallbackData->pMessage);
	}

	return VK_FALSE;
}

void FINALITY::VKRenderDevice::SetupDebugMessanger()
{
	if (!m_EnableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	VkResult res = CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessanger);
	CHECK_VK_RESULT(res, "Create Debug Messanger");
}

VkResult FINALITY::VKRenderDevice::CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void FINALITY::VKRenderDevice::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void FINALITY::VKRenderDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

void FINALITY::VKRenderDevice::CreateSurface(const NativeWindowHandle& handle)
{
	GLFWwindow* window = (GLFWwindow*)handle.WindowHandle;
	VkResult res = glfwCreateWindowSurface(m_Instance, window, nullptr, &m_Surface);
	CHECK_VK_RESULT(res, "Vulkan Surface Creation");
}

void FINALITY::VKRenderDevice::DestroySurface()
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

void FINALITY::VKRenderDevice::Initialize(const NativeWindowHandle& handle)
{
	this->CreateInstance();
	this->SetupDebugMessanger();
	this->CreateSurface(handle);

	m_Devices.Initialize(m_Instance, m_Surface);
	m_QueueFamily = m_Devices.SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);

	this->CreateDevice();
	this->CreateSwapChain();
}

void FINALITY::VKRenderDevice::Shutdown()
{
	for (size_t i = 0; i < m_ImageViews.size(); i++)
	{
		vkDestroyImageView(m_Device, m_ImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);

	DestroyDevice();

	if (m_EnableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessanger, nullptr);
	}

	DestroySurface();
	vkDestroyInstance(m_Instance, nullptr);
}

void FINALITY::VKRenderDevice::BeginFrame()
{
}

void FINALITY::VKRenderDevice::EndFrame()
{
}

void FINALITY::VKRenderDevice::PresentFrame()
{
}

void FINALITY::VKRenderDevice::Clear(float r, float g, float b, float a)
{
}
