#pragma once
#include <memory>
#include "Framebuffer.h"
#include <Renderer/Camera.h>

namespace FINALITY
{
	struct RenderTarget
	{
		virtual ~RenderTarget() = default;

		std::shared_ptr<Framebuffer> Framebuffer;
		Camera* SourceCamera = nullptr;
		uint32_t Width = 0;
		uint32_t Height = 0;

		bool ResizeRequested = false;
		uint32_t PendingWidth = 0;
		uint32_t PendingHeight = 0;

		glm::vec4 ClearColor{ 0.1f, 0.1f, 0.1f, 1.0f };
	};
}