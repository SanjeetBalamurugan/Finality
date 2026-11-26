/**
 * @file
 * @ingroup core
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for RenderTypes.h.
 */
#pragma once
#include <glm/glm.hpp>

namespace FINALITY
{
    /** @todo TODO: Document RendererAPI enum semantics, ownership and usage. */
    enum RendererAPI
    {
        VULKAN,

        NONE
    };

    /** @todo TODO: Document GlobalUniformBufferObject struct semantics, ownership and usage. */
    struct GlobalUniformBufferObject
    {
        glm::mat4 View;
        glm::mat4 Projection;
    };
} // namespace FINALITY