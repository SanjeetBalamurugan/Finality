#include "Application.h"
#include <iostream>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "GLFWWindowImpl.h"

// Vulkan Specific
#include <Vulkan/VKRenderDevice.h>
//

#include "Log.h"

void FINALITY::Application::Initialize(const RendererAPI& api, const WindowSpec& spec)
{
	Logger::Init();

	FI_CORE_INFO("FINALITY ENGINE");

	if (!glfwInit())
	{
		FI_CORE_ERROR("GLFW Cannot Initialise!!");
		std::exit(EXIT_FAILURE);
	}

	if (!glfwVulkanSupported() && api == RendererAPI::VULKAN)
	{
		FI_CORE_ERROR("VULKAN is not supported on this system");
		std::exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	switch (api)
	{
	case RendererAPI::VULKAN:
		m_RenderDevice = std::make_unique<VKRenderDevice>();
		break;
	default:
		FI_CORE_ERROR("UNKNOWN API!!!");
		std::exit(EXIT_FAILURE);
		break;
	}

	m_Window = std::make_unique<GLFWWindowImpl>();
	m_Window->Initialize(spec);

	m_RenderDevice->Initialize(m_Window->GetNativeHandles());
	m_RenderDevice->SetWindowSpec(spec);

	m_Running = true;
}

void FINALITY::Application::Update()
{
	while (!m_Window->ShouldClose() && m_Running)
	{
		m_Window->Update();

		m_RenderDevice->BeginFrame();

		// Frame logic like draw calls here

		m_RenderDevice->Clear(0.1f, 0.8f, 0.3f, 1.0f);

		m_RenderDevice->EndFrame();
		m_RenderDevice->PresentFrame();
	}

	m_Running = false;
}

void FINALITY::Application::Shutdown()
{
	if (m_Window) m_Window->Shutdown();
	if (m_RenderDevice) m_RenderDevice->Shutdown();
	if (!m_Running) glfwTerminate();
}
