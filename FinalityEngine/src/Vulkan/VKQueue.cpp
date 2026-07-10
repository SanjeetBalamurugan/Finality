#include "VKQueue.h"
#include <stdexcept>

namespace FINALITY
{
    void VKQueue::Initialize(VkDevice device, VkSwapchainKHR swapchain, uint32_t queueFamily, uint32_t queueIndex)
    {
        m_Device = device;
        m_SwapChain = swapchain;
        vkGetDeviceQueue(m_Device, queueFamily, queueIndex, &m_Queue);

        uint32_t imageCount = 3;
        CreateSemaphores(imageCount);
    }

    void VKQueue::CreateSemaphores(uint32_t swapchainImageCount)
    {
        m_InFlightFences.resize(swapchainImageCount);
        m_ImageAvailableSemaphores.resize(swapchainImageCount);
        m_RenderFinishedSemaphores.resize(swapchainImageCount);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < swapchainImageCount; i++)
        {
            if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create multi-buffered GPU synchronization objects!");
            }
        }
    }

    uint32_t VKQueue::AcquireNextImage()
    {
        uint32_t nextAvailableSlot = 0;
        for (size_t i = 0; i < m_InFlightFences.size(); i++)
        {
            if (vkGetFenceStatus(m_Device, m_InFlightFences[i]) == VK_SUCCESS)
            {
                nextAvailableSlot = static_cast<uint32_t>(i);
                break;
            }
        }

        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(
            m_Device,
            m_SwapChain,
            UINT64_MAX,
            m_ImageAvailableSemaphores[nextAvailableSlot],
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return UINT32_MAX;
        }

        if (nextAvailableSlot != imageIndex)
        {
            std::swap(m_ImageAvailableSemaphores[nextAvailableSlot], m_ImageAvailableSemaphores[imageIndex]);
        }

        vkWaitForFences(m_Device, 1, &m_InFlightFences[imageIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device, 1, &m_InFlightFences[imageIndex]);

        return imageIndex;
    }

    void VKQueue::SubmitASync(VkCommandBuffer cmdBuf, uint32_t imageIndex)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[imageIndex] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuf;

        VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[imageIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult res = vkQueueSubmit(m_Queue, 1, &submitInfo, m_InFlightFences[imageIndex]);
        CHECK_VK_RESULT(res, "vkQueueSubmit frame execution failure");
    }

    void VKQueue::Present(uint32_t imageIndex)
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

        vkQueuePresentKHR(m_Queue, &presentInfo);
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
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        }
    }
}
