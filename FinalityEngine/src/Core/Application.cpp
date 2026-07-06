#include "Application.h"
#include <iostream>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "GLFWWindowImpl.h"

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

	m_Window = std::make_unique<GLFWWindowImpl>();
	m_Window->Initialize(spec);
}

void FINALITY::Application::Update()
{
	while (!m_Window->ShouldClose())
	{
		m_Window->Update();
	}
}

void FINALITY::Application::Shutdown()
{
	m_Window->Shutdown();
	if (!m_Running)
		glfwTerminate();
}
