#include "GLFWWindowImpl.h"
#include "Log.h"
#include "Events/Input.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <Events/Mouse.h>

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

	glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xpos, double ypos) {
		FINALITY::Mouse::RecordMousePosition(static_cast<float>(xpos), static_cast<float>(ypos));
		});

	glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
		if (button >= 0 && button < 8) {
			if (action == GLFW_PRESS) {
				FINALITY::Mouse::RecordButtonPress(static_cast<MouseCode>(button));
			}
			else if (action == GLFW_RELEASE) {
				FINALITY::Mouse::RecordButtonRelease(static_cast<MouseCode>(button));
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

void FINALITY::GLFWWindowImpl::SetCursorMode(bool hiddenAndLocked)
{
	if (!m_Window) return;

	if (hiddenAndLocked)
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void FINALITY::GLFWWindowImpl::SetWindowTitle(const std::string& title)
{
	if (m_Window)
	{
		glfwSetWindowTitle(m_Window, title.c_str());
	}
}
