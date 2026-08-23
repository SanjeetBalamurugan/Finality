#include "VKQueue.h"
#include <stdexcept>

namespace FINALITY
{
    void VKQueue::Initialize(VkDevice device, VkSwapchainKHR swapchain, uint32_t swapchainImageCount, uint32_t queueFamily, uint32_t queueIndex)
    {
        m_Device = device;
        m_SwapChain = swapchain;
        vkGetDeviceQueue(m_Device, queueFamily, queueIndex, &m_Queue);

        CreateSyncObjects(swapchainImageCount);
    }

    void VKQueue::CreateSyncObjects(uint32_t swapchainImageCount)
    {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        m_RenderFinishedSemaphores.resize(swapchainImageCount);
        m_ImagesInFlight.resize(swapchainImageCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create per-frame GPU synchronization objects!");
            }
        }

        for (size_t i = 0; i < swapchainImageCount; i++)
        {
            if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create per-image render-finished semaphores!");
            }
        }
    }

    void VKQueue::SubmitASync(VkCommandBuffer cmdBuf, uint32_t imageIndex, bool isFirst, bool isLast)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        if (isFirst) {
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
        }

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuf;

        VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[imageIndex] };
        if (isLast) {
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
        }

        VkResult res = vkQueueSubmit(m_Queue, 1, &submitInfo, isLast ? m_InFlightFences[m_CurrentFrame] : VK_NULL_HANDLE);
        CHECK_VK_RESULT(res, "vkQueueSubmit frame execution failure");
    }

    void VKQueue::WaitIdle()
    {
        vkQueueWaitIdle(m_Queue);
    }

    void VKQueue::SubmitSync(VkCommandBuffer cmdBuf)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuf;

        vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_Queue);
    }

    void VKQueue::ShutDown()
    {
        for (size_t i = 0; i < m_InFlightFences.size(); i++) {
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
        }
        for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); i++) {
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        }
    }

    uint32_t VKQueue::AcquireNextImage(bool& outNeedsRecreate)
    {
        outNeedsRecreate = false;
        vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(
            m_Device,
            m_SwapChain,
            UINT64_MAX,
            m_ImageAvailableSemaphores[m_CurrentFrame],
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            outNeedsRecreate = true;
            return UINT32_MAX;
        }

        if (result == VK_SUBOPTIMAL_KHR)
        {
            outNeedsRecreate = true;
        }
        else if (result != VK_SUCCESS)
        {
            CHECK_VK_RESULT(result, "vkAcquireNextImageKHR");
        }

        if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_Device, 1, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_ImagesInFlight[imageIndex] = m_InFlightFences[m_CurrentFrame];

        vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

        return imageIndex;
    }

    bool VKQueue::Present(uint32_t imageIndex)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore waitSemaphores[] = { m_RenderFinishedSemaphores[imageIndex] };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = waitSemaphores;

        VkSwapchainKHR swapChains[] = { m_SwapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(m_Queue, &presentInfo);

        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            return true;
        }

        CHECK_VK_RESULT(result, "vkQueuePresentKHR");
        return false;
    }
}

