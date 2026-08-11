#include "VKRenderDevice.h"
#include <vector>
#include <GLFW/glfw3.h>
#include "VkHelpers.h"
#include "VKDebug.h"
#include "VKMesh.h"
#include "VKPipeline.h"
#include <glm/ext/matrix_transform.hpp>
#include <Renderer/Renderer.h>
#include "VKTexture.h"
#include "VKFramebuffer.h"

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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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

	m_Queue.Initialize(m_Device, m_SwapChain->GetVKHandle(), m_SwapChain->GetImageCount() , m_QueueFamily, 0);
	this->CreateCommandBuffers(m_SwapChain->GetImageCount());

	m_FrameDeletionQueues.resize(m_SwapChain->GetImageCount());
	m_MaterialDescriptorAllocator.Initialize(m_Device);

	uint32_t imageCount = m_SwapChain->GetImageCount();

	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	VkResult res = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_GlobalDescriptorSetLayout);
	CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout");

	m_GlobalUBO.Initialize(m_Device, m_Devices.SelectedDevice().device, sizeof(GlobalUniformBufferObject), imageCount);

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = imageCount;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = imageCount;

	res = vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_GlobalDescriptorPool);
	CHECK_VK_RESULT(res, "vkCreateDescriptorPool");

	std::vector<VkDescriptorSetLayout> layouts(imageCount, m_GlobalDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_GlobalDescriptorPool;
	allocInfo.descriptorSetCount = imageCount;
	allocInfo.pSetLayouts = layouts.data();

	m_GlobalDescriptorSets.resize(imageCount);
	res = vkAllocateDescriptorSets(m_Device, &allocInfo, m_GlobalDescriptorSets.data());
	CHECK_VK_RESULT(res, "vkAllocateDescriptorSets");

	for (uint32_t i = 0; i < imageCount; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_GlobalUBO.GetBuffer(i);
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(GlobalUniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_GlobalDescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
	}

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
	materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	materialLayoutInfo.bindingCount = 1;
	materialLayoutInfo.pBindings = &samplerLayoutBinding;

	res = vkCreateDescriptorSetLayout(m_Device, &materialLayoutInfo, nullptr, &m_MaterialDescriptorSetLayout);
	CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout for Material");

	FramebufferSpecification fbSpec{};
	fbSpec.Width = m_SwapChain->GetWidth();
	fbSpec.Height = m_SwapChain->GetHeight();
	fbSpec.IsSwapChainTarget = false;

	m_PostProcessingFramebuffer = Framebuffer::Create(fbSpec);

	VkDescriptorSetLayoutBinding samplerBinding{};
	samplerBinding.binding = 0;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.descriptorCount = 1;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo ppLayoutInfo{};
	ppLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ppLayoutInfo.bindingCount = 1;
	ppLayoutInfo.pBindings = &samplerBinding;
	vkCreateDescriptorSetLayout(m_Device, &ppLayoutInfo, nullptr, &m_PostProcessDescriptorSetLayout);

	VkDescriptorPoolSize ppPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
	VkDescriptorPoolCreateInfo ppPoolInfo{};
	ppPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ppPoolInfo.poolSizeCount = 1;
	ppPoolInfo.pPoolSizes = &ppPoolSize;
	ppPoolInfo.maxSets = 1;
	vkCreateDescriptorPool(m_Device, &ppPoolInfo, nullptr, &m_PostProcessDescriptorPool);

	VkDescriptorSetAllocateInfo ppAllocInfo{};
	ppAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ppAllocInfo.descriptorPool = m_PostProcessDescriptorPool;
	ppAllocInfo.descriptorSetCount = 1;
	ppAllocInfo.pSetLayouts = &m_PostProcessDescriptorSetLayout;
	vkAllocateDescriptorSets(m_Device, &ppAllocInfo, &m_PostProcessDescriptorSet);

	auto* vkFB = static_cast<VKFramebuffer*>(m_PostProcessingFramebuffer.get());
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = (VkImageView)vkFB->GetColorAttachmentRendererID();

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	vkCreateSampler(m_Device, &samplerCreateInfo, nullptr, &m_PostProcessSampler);
	imageInfo.sampler = m_PostProcessSampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = m_PostProcessDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
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

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				vkPipeline->GetVKLayout(),
				0, 1,
				&m_GlobalDescriptorSets[m_ImageIndex],
				0, nullptr
			);
		}

		if (!packet.Textures.empty())
		{
			const void* cacheKey = &packet.Textures;
			auto it = m_MaterialDescriptorCache.find(static_cast<const Material*>(cacheKey));
			VkDescriptorSet materialSet = VK_NULL_HANDLE;

			if (it == m_MaterialDescriptorCache.end())
			{
				materialSet = m_MaterialDescriptorAllocator.Allocate(m_MaterialDescriptorSetLayout);
				m_MaterialDescriptorCache[static_cast<const Material*>(cacheKey)] = materialSet;

				auto texIt = packet.Textures.begin();
				auto* vkTex = static_cast<VKTexture*>(texIt->second.get());

				if (vkTex)
				{
					VkDescriptorImageInfo imageInfo{};
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo.imageView = vkTex->GetImageView();
					imageInfo.sampler = vkTex->GetSampler();

					VkWriteDescriptorSet descriptorWrite{};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = materialSet;
					descriptorWrite.dstBinding = 0;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
				}
			}
			else
			{
				materialSet = it->second;
			}

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				vkPipeline->GetVKLayout(),
				1, 1,
				&materialSet,
				0, nullptr
			);
		}

		uint8_t pushBuffer[128] = { 0 };
		std::memcpy(pushBuffer, &packet.Transform, sizeof(glm::mat4));

		if (!packet.CustomPushData.empty())
		{
			std::memcpy(pushBuffer + 64, packet.CustomPushData.data() + 64, 64);
		}

		vkCmdPushConstants(
			cmd,
			vkPipeline->GetVKLayout(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			128,
			pushBuffer
		);

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
	return std::make_shared<VKPipeline>(m_Device, GetActiveRenderPass(), config, m_GlobalDescriptorSetLayout, m_MaterialDescriptorSetLayout);
}

void FINALITY::VKRenderDevice::BeginTextureBatch()
{
	m_ActiveUploadBatch.isActive = true;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CMDBufPool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(m_Device, &allocInfo, &m_ActiveUploadBatch.commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(m_ActiveUploadBatch.commandBuffer, &beginInfo);
}

void FINALITY::VKRenderDevice::EndAndSubmitTextureBatch()
{
	if (!m_ActiveUploadBatch.isActive) return;

	vkEndCommandBuffer(m_ActiveUploadBatch.commandBuffer);

	// Submit the monolithic batch once
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_ActiveUploadBatch.commandBuffer;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence fence;
	vkCreateFence(m_Device, &fenceInfo, nullptr, &fence);

	vkQueueSubmit(m_Queue.GetQueue(), 1, &submitInfo, fence);
	vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX);

	for (size_t i = 0; i < m_ActiveUploadBatch.stagingBuffers.size(); ++i) {
		vkDestroyBuffer(m_Device, m_ActiveUploadBatch.stagingBuffers[i], nullptr);
		vkFreeMemory(m_Device, m_ActiveUploadBatch.stagingMemories[i], nullptr);
	}

	vkDestroyFence(m_Device, fence, nullptr);
	vkFreeCommandBuffers(m_Device, m_CMDBufPool, 1, &m_ActiveUploadBatch.commandBuffer);

	m_ActiveUploadBatch = TextureUploadBatchContext();
}

void FINALITY::VKRenderDevice::Shutdown()
{
	m_Queue.WaitIdle();

	m_PostProcessPipeline.reset();
	m_PostProcessingFramebuffer.reset();

	if (m_PostProcessDescriptorPool) {
		vkDestroyDescriptorPool(m_Device, m_PostProcessDescriptorPool, nullptr);
		m_PostProcessDescriptorPool = VK_NULL_HANDLE;
	}
	if (m_PostProcessSampler) vkDestroySampler(m_Device, m_PostProcessSampler, nullptr);
	if (m_PostProcessDescriptorSetLayout) {
		vkDestroyDescriptorSetLayout(m_Device, m_PostProcessDescriptorSetLayout, nullptr);
		m_PostProcessDescriptorSetLayout = VK_NULL_HANDLE;
	}

	m_MaterialDescriptorAllocator.Shutdown();

	for (auto& queue : m_FrameDeletionQueues)
	{
		queue.Flush();
	}

	if (m_MaterialDescriptorSetLayout) vkDestroyDescriptorSetLayout(m_Device, m_MaterialDescriptorSetLayout, nullptr);

	if (m_GlobalDescriptorPool) vkDestroyDescriptorPool(m_Device, m_GlobalDescriptorPool, nullptr);
	if (m_GlobalDescriptorSetLayout) vkDestroyDescriptorSetLayout(m_Device, m_GlobalDescriptorSetLayout, nullptr);
	m_GlobalUBO.Shutdown();

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

	m_FrameDeletionQueues[m_ImageIndex].Flush();

	if (Renderer::GetActiveCamera())
	{
		GlobalUniformBufferObject ubo{};
		ubo.View = Renderer::GetActiveCamera()->GetViewMatrix();
		ubo.Projection = Renderer::GetActiveCamera()->GetProjection();

		m_GlobalUBO.Update(m_ImageIndex, &ubo);
	}

	BeginCommandBuffers(m_CMDBuffers[m_ImageIndex], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	std::vector<VkClearValue> clearValues(2);
	clearValues[0].color = m_ClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	auto* vkFB = static_cast<VKFramebuffer*>(m_PostProcessingFramebuffer.get());

	VkRenderPassBeginInfo RenderPassBeginInfo{};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassBeginInfo.pNext = nullptr;
	RenderPassBeginInfo.renderPass = vkFB->GetVKRenderPass();
	RenderPassBeginInfo.framebuffer = vkFB->GetVKFramebuffer();
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };
	RenderPassBeginInfo.renderArea.extent.width = m_PostProcessingFramebuffer->GetSpecification().Width;
	RenderPassBeginInfo.renderArea.extent.height = m_PostProcessingFramebuffer->GetSpecification().Height;
	RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	RenderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_CMDBuffers[m_ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void FINALITY::VKRenderDevice::EndFrame()
{
	VkCommandBuffer cmd = m_CMDBuffers[m_ImageIndex];

	vkCmdEndRenderPass(cmd);

	auto* vkFB = static_cast<VKFramebuffer*>(m_PostProcessingFramebuffer.get());

	if (m_PostProcessPipeline)
	{
		VkImageMemoryBarrier scBarrier{};
		scBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		scBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		scBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		scBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		scBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		scBarrier.image = m_SwapChain->GetVKImage(m_ImageIndex);
		scBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		scBarrier.subresourceRange.baseMipLevel = 0;
		scBarrier.subresourceRange.levelCount = 1;
		scBarrier.subresourceRange.baseArrayLayer = 0;
		scBarrier.subresourceRange.layerCount = 1;
		scBarrier.srcAccessMask = 0;
		scBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &scBarrier);

		std::vector<VkClearValue> screenClearValues(2);
		screenClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		screenClearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo swapchainPassInfo{};
		swapchainPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		swapchainPassInfo.renderPass = m_SwapChain->GetVKRenderPass();
		swapchainPassInfo.framebuffer = m_SwapChain->GetVKFramebuffer(m_ImageIndex);
		swapchainPassInfo.renderArea.offset = { 0, 0 };
		swapchainPassInfo.renderArea.extent.width = m_SwapChain->GetWidth();
		swapchainPassInfo.renderArea.extent.height = m_SwapChain->GetHeight();
		swapchainPassInfo.clearValueCount = static_cast<uint32_t>(screenClearValues.size());
		swapchainPassInfo.pClearValues = screenClearValues.data();

		vkCmdBeginRenderPass(cmd, &swapchainPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		auto* vkPipeline = static_cast<VKPipeline*>(m_PostProcessPipeline.get());
		vkPipeline->Bind(cmd);

		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			vkPipeline->GetVKLayout(),
			0, 1,
			&m_PostProcessDescriptorSet,
			0, nullptr
		);

		VkViewport viewport{ 0.0f, 0.0f, static_cast<float>(m_SwapChain->GetWidth()), static_cast<float>(m_SwapChain->GetHeight()), 0.0f, 1.0f };
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{ {0, 0}, {m_SwapChain->GetWidth(), m_SwapChain->GetHeight()} };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdDraw(cmd, 3, 1, 0, 0);
		vkCmdEndRenderPass(cmd);
	}
	else
	{
		VkImageMemoryBarrier fbBarrier{};
		fbBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		fbBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		fbBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		fbBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		fbBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		fbBarrier.image = vkFB->GetVKColorImage();
		fbBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		fbBarrier.subresourceRange.baseMipLevel = 0;
		fbBarrier.subresourceRange.levelCount = 1;
		fbBarrier.subresourceRange.baseArrayLayer = 0;
		fbBarrier.subresourceRange.layerCount = 1;
		fbBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		fbBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		VkImageMemoryBarrier scBarrier{};
		scBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		scBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		scBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		scBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		scBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		scBarrier.image = m_SwapChain->GetVKImage(m_ImageIndex);
		scBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		scBarrier.subresourceRange.baseMipLevel = 0;
		scBarrier.subresourceRange.levelCount = 1;
		scBarrier.subresourceRange.baseArrayLayer = 0;
		scBarrier.subresourceRange.layerCount = 1;
		scBarrier.srcAccessMask = 0;
		scBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &fbBarrier);
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &scBarrier);

		VkImageBlit blitRegion{};
		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.mipLevel = 0;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcOffsets[0] = { 0, 0, 0 };
		blitRegion.srcOffsets[1] = { static_cast<int32_t>(m_PostProcessingFramebuffer->GetSpecification().Width), static_cast<int32_t>(m_PostProcessingFramebuffer->GetSpecification().Height), 1 };

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.mipLevel = 0;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstOffsets[0] = { 0, 0, 0 };
		blitRegion.dstOffsets[1] = { static_cast<int32_t>(m_SwapChain->GetWidth()), static_cast<int32_t>(m_SwapChain->GetHeight()), 1 };

		vkCmdBlitImage(
			cmd,
			vkFB->GetVKColorImage(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_SwapChain->GetVKImage(m_ImageIndex),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blitRegion,
			VK_FILTER_LINEAR
		);

		VkImageMemoryBarrier presentBarrier{};
		presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		presentBarrier.image = m_SwapChain->GetVKImage(m_ImageIndex);
		presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		presentBarrier.subresourceRange.baseMipLevel = 0;
		presentBarrier.subresourceRange.levelCount = 1;
		presentBarrier.subresourceRange.baseArrayLayer = 0;
		presentBarrier.subresourceRange.layerCount = 1;
		presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		presentBarrier.dstAccessMask = 0;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);
	}

	VkResult res = vkEndCommandBuffer(cmd);
	CHECK_VK_RESULT(res, "vkEndCommandBuffer error");

	m_Queue.SubmitASync(cmd, m_ImageIndex);
}

void FINALITY::VKRenderDevice::PresentFrame()
{
	m_Queue.Present(m_ImageIndex);
}

void FINALITY::VKRenderDevice::Clear(float r, float g, float b, float a)
{
	m_ClearColor = { .float32 = { r, g, b, a } };
}
