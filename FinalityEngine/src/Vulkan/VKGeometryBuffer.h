/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKGeometryBuffer.h.
 */
#pragma once
#include "VKCore.h"
#include <memory>

namespace FINALITY
{
    class VKRenderDevice;

    /** @todo TODO: Document VKGeometryBuffer class semantics, ownership and usage. */
    class VKGeometryBuffer
    {
    public:
        ~VKGeometryBuffer();
        VkBuffer Buffer = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;
        static std::shared_ptr<VKGeometryBuffer> Upload(VKRenderDevice& device, const void* data, VkDeviceSize size, VkBufferUsageFlags usage);
    };

    /** @todo TODO: Document VKGeometryUploadScope class semantics, ownership and usage. */
    class VKGeometryUploadScope
    {
    public:
        explicit VKGeometryUploadScope(VKRenderDevice& device);
        ~VKGeometryUploadScope();

    private:
        VKRenderDevice& m_Device;
        bool m_OwnsBatch;
    };
} // namespace FINALITY
