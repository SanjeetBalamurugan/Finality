#include "GLFWWindowImpl.h"
#include "Log.h"
#include "Events/Input.h"

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

	m_NativeHandle.WindowHandle = static_cast<void*>(m_Window);
	m_NativeHandle.DeviceContext = static_cast<void*>(glfwGetWin32Window(m_Window));

	glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (key >= 0 && key < 512) {
			if (action == GLFW_PRESS) {
				FINALITY::Input::RecordKeyPress(static_cast<KeyCode>(key));
			}
			else if (action == GLFW_RELEASE) {
				FINALITY::Input::RecordKeyRelease(static_cast<KeyCode>(key));
			}
		}
		});
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
