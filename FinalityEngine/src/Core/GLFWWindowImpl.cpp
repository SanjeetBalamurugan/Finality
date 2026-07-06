#include "GLFWWindowImpl.h"
#include "Log.h"

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
