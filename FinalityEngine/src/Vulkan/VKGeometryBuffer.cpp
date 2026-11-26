#include "VKGeometryBuffer.h"
#include "VKRenderDevice.h"
#include <cstring>
#include <stdexcept>

namespace
{
    void Allocate(FINALITY::VKGeometryBuffer& allocation, VkPhysicalDevice physical, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags)
    {
        VkBufferCreateInfo info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        info.size = size;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(allocation.Device, &info, nullptr, &allocation.Buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Geometry buffer creation failed");
        }
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(allocation.Device, allocation.Buffer, &requirements);
        VkPhysicalDeviceMemoryProperties properties;
        vkGetPhysicalDeviceMemoryProperties(physical, &properties);
        uint32_t type = 0;
        while (type < properties.memoryTypeCount && (!(requirements.memoryTypeBits & (1u << type)) || (properties.memoryTypes[type].propertyFlags & flags) != flags))
        {
            ++type;
        }
        if (type == properties.memoryTypeCount)
        {
            throw std::runtime_error("No compatible geometry memory type");
        }
        VkMemoryAllocateInfo memory{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memory.allocationSize = requirements.size;
        memory.memoryTypeIndex = type;
        if (vkAllocateMemory(allocation.Device, &memory, nullptr, &allocation.Memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Geometry allocation failed");
        }
        if (vkBindBufferMemory(allocation.Device, allocation.Buffer, allocation.Memory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Geometry binding failed");
        }
    }
} // namespace

FINALITY::VKGeometryBuffer::~VKGeometryBuffer()
{
    if (Buffer)
    {
        vkDestroyBuffer(Device, Buffer, nullptr);
    }
    if (Memory)
    {
        vkFreeMemory(Device, Memory, nullptr);
    }
}

std::shared_ptr<FINALITY::VKGeometryBuffer> FINALITY::VKGeometryBuffer::Upload(VKRenderDevice& device, const void* data, VkDeviceSize size, VkBufferUsageFlags usage)
{
    if (!size)
    {
        return {};
    }
    if (!data || !device.IsUploadBatchActive())
    {
        throw std::runtime_error("Geometry upload requires data and an active upload batch");
    }
    auto result = std::make_shared<VKGeometryBuffer>();
    result->Device = device.GetActiveDevice();
    Allocate(*result, device.GetActivePhysicalDevice(), size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    auto staging = std::make_shared<VKGeometryBuffer>();
    staging->Device = result->Device;
    Allocate(*staging, device.GetActivePhysicalDevice(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* mapped = nullptr;
    if (vkMapMemory(staging->Device, staging->Memory, 0, size, 0, &mapped) != VK_SUCCESS)
    {
        throw std::runtime_error("Geometry staging map failed");
    }
    std::memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(staging->Device, staging->Memory);
    device.RetainUploadResource(result);
    device.RetainUploadResource(staging);
    const VkBufferCopy copy{ 0, 0, size };
    const auto command = device.GetActiveUploadCommandBuffer();
    vkCmdCopyBuffer(command, staging->Buffer, result->Buffer, 1, &copy);
    VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT ? VK_ACCESS_INDEX_READ_BIT : VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = result->Buffer;
    barrier.size = size;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    return result;
}

FINALITY::VKGeometryUploadScope::VKGeometryUploadScope(VKRenderDevice& device)
    : m_Device(device), m_OwnsBatch(!device.IsUploadBatchActive())
{
    if (m_OwnsBatch)
    {
        m_Device.BeginTextureBatch();
    }
}

FINALITY::VKGeometryUploadScope::~VKGeometryUploadScope()
{
    if (m_OwnsBatch)
    {
        m_Device.EndAndSubmitTextureBatch();
    }
}
