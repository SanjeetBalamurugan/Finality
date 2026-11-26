/**
 * @file
 * @ingroup core
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for RenderTarget.h.
 */
#pragma once
#include <memory>
#include "Framebuffer.h"
#include <Renderer/Camera.h>
#include <Renderer/RenderStatistics.h>
#include "RenderView.h"
#include <Renderer/LODSelection.h>

namespace FINALITY
{
    /** @todo TODO: Document RenderTarget struct semantics, ownership and usage. */
    struct RenderTarget
    {
        virtual ~RenderTarget() = default;

        std::shared_ptr<Framebuffer> Framebuffer;
        uint32_t Width = 0;
        uint32_t Height = 0;
        RenderView View;
        RenderStatistics Statistics;
        ViewVisibilityState VisibilityState;

        bool ResizeRequested = false;
        uint32_t PendingWidth = 0;
        uint32_t PendingHeight = 0;

        glm::vec4 ClearColor{ 0.1f, 0.1f, 0.1f, 1.0f };
    };
} // namespace FINALITY
