#include "Texture.h"
#include "Application.h"


#include <Vulkan/VKRenderDevice.h>
#include <Vulkan/VKTexture.h>

std::shared_ptr<FINALITY::Texture> FINALITY::Texture::Create(const std::string& filePath)
{
    switch (FINALITY::Application::GetActiveAPI())
    {
    case RendererAPI::NONE:
        throw std::runtime_error("Renderer API is currently set to None!");
        return nullptr;

    case RendererAPI::VULKAN:
    {
        auto* renderDevice = static_cast<VKRenderDevice*>(&FINALITY::Application::Get().GetDevice());
        if (!renderDevice) throw std::runtime_error("Vulkan Render Device is invalid!");

        VkDevice device = renderDevice->GetActiveDevice();
        VkCommandBuffer targetCmd = VK_NULL_HANDLE;
        bool isSharedBatch = renderDevice->IsUploadBatchActive();

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        if (isSharedBatch)
        {
            targetCmd = renderDevice->GetActiveUploadCommandBuffer();
        }
        else
        {
            VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = renderDevice->GetCommandPool();
            allocInfo.commandBufferCount = 1;
            vkAllocateCommandBuffers(device, &allocInfo, &targetCmd);

            VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(targetCmd, &beginInfo);
        }

        auto texture = std::make_shared<VKTexture>(
            device,
            renderDevice->GetActivePhysicalDevice(),
            targetCmd,
            filePath,
            stagingBuffer,
            stagingMemory
        );

        if (isSharedBatch)
        {
            renderDevice->TrackStagingResource(stagingBuffer, stagingMemory);
        }
        else
        {
            vkEndCommandBuffer(targetCmd);

            VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &targetCmd;

            VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            VkFence fence;
            vkCreateFence(device, &fenceInfo, nullptr, &fence);

            vkQueueSubmit(renderDevice->GetGraphicsQueue(), 1, &submitInfo, fence);
            vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

            vkDestroyFence(device, fence, nullptr);
            vkFreeCommandBuffers(device, renderDevice->GetCommandPool(), 1, &targetCmd);

            if (stagingBuffer) vkDestroyBuffer(device, stagingBuffer, nullptr);
            if (stagingMemory) vkFreeMemory(device, stagingMemory, nullptr);
        }

        return texture;
    }
    default:
        throw std::runtime_error("Unknown or unsupported graphics API configuration layer!");
        return nullptr;
    }
}
