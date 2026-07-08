#include "VKQueue.h"
#include "VkHelpers.h"

void FINALITY::VKQueue::CreateSemaphores()
{
	m_PresentCompleteSem = VkCreateSemaphore(m_Device);
	m_RenderCompleteSem = VkCreateSemaphore(m_Device);
}

void FINALITY::VKQueue::Initialize(VkDevice device, VkSwapchainKHR swapchain, uint32_t queueFamily, uint32_t queueIndex)
{
	m_Device = device;
	m_SwapChain = swapchain;

	vkGetDeviceQueue(m_Device, queueFamily, queueIndex, &m_Queue);
	this->CreateSemaphores();
}

void FINALITY::VKQueue::ShutDown()
{
	vkDestroySemaphore(m_Device, m_PresentCompleteSem, nullptr);
	vkDestroySemaphore(m_Device, m_RenderCompleteSem, nullptr);
}

uint32_t FINALITY::VKQueue::AcquireNextImage()
{
	uint32_t imageIdx = 0;
	VkResult res = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_PresentCompleteSem, nullptr, &imageIdx);
	CHECK_VK_RESULT(res, "vkAcquireNextImageKHR error");

	return imageIdx;
}

void FINALITY::VKQueue::SubmitSync(VkCommandBuffer cmdBuf)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = nullptr;

	VkResult res = vkQueueSubmit(m_Queue, 1, &submitInfo, nullptr);
	CHECK_VK_RESULT(res, "vkQueueSubmit error(VKQueue::SubmitSync)");
}

void FINALITY::VKQueue::SubmitASync(VkCommandBuffer cmdBuf)
{
	VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_PresentCompleteSem;
	submitInfo.pWaitDstStageMask = &waitFlags;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_RenderCompleteSem;

	VkResult res = vkQueueSubmit(m_Queue, 1, &submitInfo, nullptr);
	CHECK_VK_RESULT(res, "vkQueueSubmit error(VKQueue::SubmitASync)");
}

void FINALITY::VKQueue::Present(uint32_t imageIndex)
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderCompleteSem;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_SwapChain;
	presentInfo.pImageIndices = &imageIndex;

	VkResult res = vkQueuePresentKHR(m_Queue, &presentInfo);
	CHECK_VK_RESULT(res, "vkQueuePresentKHR error");
}

void FINALITY::VKQueue::WaitIdle()
{
	vkQueueWaitIdle(m_Queue);
}
