#pragma once
#include "Core.h"
#include "Window.h"

#include <GLFW/glfw3.h>

namespace FINALITY
{
	class GLFWWindowImpl : public Window
	{
	public:
		void Initialize(const WindowSpec& specification) override;
		void Update() override;
		void Shutdown() override;

		bool ShouldClose() const override;
		uint32_t GetWidth() const override { return m_Specification.width; }
		uint32_t GetHeight() const override { return m_Specification.height; }
		WindowSpec GetSpecifications() const override { return m_Specification; }
		NativeWindowHandle GetNativeHandles() const override { return m_NativeHandle; }
	private:
		GLFWwindow* m_Window = nullptr;
		WindowSpec m_Specification;

		NativeWindowHandle m_NativeHandle;
	};
}