#pragma once
#include "VKCore.h"
#include <vector>

namespace FINALITY
{
	class VKQueue
	{
	private:
		void CreateSemaphores();
	public:
		void Initialize(VkDevice device, VkSwapchainKHR swapchain, uint32_t queueFamily, uint32_t queueIndex);
		void ShutDown();

		void UpdateSwapChain(VkSwapchainKHR newSwapchain) { m_SwapChain = newSwapchain; }

		uint32_t AcquireNextImage();

		void SubmitSync(VkCommandBuffer cmdBuf);
		void SubmitASync(VkCommandBuffer cmdBuf);

		void Present(uint32_t imageIndex);
		void WaitIdle();

	private:
		VkDevice m_Device = nullptr;
		VkSwapchainKHR m_SwapChain = nullptr;
		VkQueue m_Queue = nullptr;

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
		size_t m_CurrentFrame = 0;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
	};
}
