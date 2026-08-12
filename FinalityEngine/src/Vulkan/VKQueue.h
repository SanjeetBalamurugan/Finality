#pragma once
#include "VKCore.h"
#include <vector>

namespace FINALITY
{
	class VKQueue
	{
	private:
		void CreateSyncObjects(uint32_t swapchainImageCount);
	public:
		void Initialize(VkDevice device, VkSwapchainKHR swapchain, uint32_t swapchainImageCount, uint32_t queueFamily, uint32_t queueIndex);
		void ShutDown();

		void UpdateSwapChain(VkSwapchainKHR newSwapchain) { m_SwapChain = newSwapchain; }

		uint32_t AcquireNextImage();

		void SubmitSync(VkCommandBuffer cmdBuf);
		void SubmitASync(VkCommandBuffer cmdBuf, uint32_t imageIndex, bool isFirst, bool isLast);

		void Present(uint32_t imageIndex);
		void WaitIdle();

		VkQueue GetQueue() { return m_Queue; }

	private:
		VkDevice m_Device = nullptr;
		VkSwapchainKHR m_SwapChain = nullptr;
		VkQueue m_Queue = nullptr;

		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		uint32_t m_CurrentFrame = 0;

		// Per frame-in-flight (size MAX_FRAMES_IN_FLIGHT)
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkFence> m_InFlightFences;

		std::vector<VkSemaphore> m_RenderFinishedSemaphores;

		std::vector<VkFence> m_ImagesInFlight;
	};
}