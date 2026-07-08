#pragma once
#include "VKCore.h"

namespace FINALITY
{
	class VKQueue
	{
	private:
		void CreateSemaphores();
	public:
		void Initialize(VkDevice device, VkSwapchainKHR swapchain, uint32_t queueFamily, uint32_t queueIndex);
		void ShutDown();

		uint32_t AcquireNextImage();

		void SubmitSync(VkCommandBuffer cmdBuf);
		void SubmitASync(VkCommandBuffer cmdBuf);

		void Present(uint32_t imageIndex);
		void WaitIdle();

	private:
		VkDevice m_Device = nullptr;
		VkSwapchainKHR m_SwapChain = nullptr;
		VkQueue m_Queue = nullptr;

		VkSemaphore m_RenderCompleteSem;
		VkSemaphore m_PresentCompleteSem;
	};
}