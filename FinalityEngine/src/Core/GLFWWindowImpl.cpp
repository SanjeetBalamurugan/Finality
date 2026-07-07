#include "GLFWWindowImpl.h"
#include "Log.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void FINALITY::GLFWWindowImpl::Initialize(const WindowSpec& specification)
{
	m_Specification = specification;
	m_Window = glfwCreateWindow(
		m_Specification.width,
		m_Specification.height,
		m_Specification.title.c_str(),
		nullptr,
		nullptr
	);

	if (!m_Window)
	{
		FI_CORE_ERROR("Window Cannot Be Created!!!");
		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}

	// Create Native Handles
	m_NativeHandle.WindowHandle = static_cast<void*>(m_Window);
	m_NativeHandle.DeviceContext = static_cast<void*>(glfwGetWin32Window(m_Window));
}

void FINALITY::GLFWWindowImpl::Update()
{
	glfwPollEvents();
}

void FINALITY::GLFWWindowImpl::Shutdown()
{
	if (m_Window) glfwDestroyWindow(m_Window);
}

bool FINALITY::GLFWWindowImpl::ShouldClose() const
{
	return glfwWindowShouldClose(m_Window);
}
