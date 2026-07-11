#pragma once
#include <glm/glm.hpp>

namespace FINALITY
{
	enum RendererAPI
	{
		VULKAN,

		NONE
	};

	struct GlobalUniformBufferObject
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};
}