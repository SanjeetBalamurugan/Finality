#pragma once
#include <imgui.h>

#include <Core/RenderTypes.h>
#include <Core/Window.h>

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>

#ifdef FINALITY_INCLUDE_VULKAN
#include <backends/imgui_impl_vulkan.h>
#endif
#include <Core/Application.h>
#include <Core/GLFWWindowImpl.h>

namespace FINALITY
{
	inline void ImGuiImplGLFW(RendererAPI api, bool installCallbacks)
	{
		GLFWWindowImpl* wImpl = static_cast<GLFWWindowImpl*>(&Application::Get().GetWindow());
		GLFWwindow* window = wImpl->GetGLFWwindow();

		switch (api)
		{
		case RendererAPI::VULKAN:
		{
#ifdef FINALITY_INCLUDE_VULKAN
			ImGui_ImplGlfw_InitForVulkan(window, installCallbacks);
#endif
			break;
		}
		default:
		{
			assert(0);
			break;
		}
		}
	}
}