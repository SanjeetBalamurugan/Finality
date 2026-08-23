#pragma once
#include "Core.h"
#include "Window.h"

#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h> 
#include <windows.h>
#endif

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

		GLFWwindow* GetGLFWwindow() const { return m_Window; }

		void SetCallbacks() override;
		bool ConsumePendingResize(int& outW, int& outH) override;

		void SetCursorMode(bool hiddenAndLocked) override;
		void SetWindowTitle(const std::string& title) override;

	private:
		GLFWwindow* m_Window = nullptr;
		WindowSpec m_Specification;

		NativeWindowHandle m_NativeHandle;

		bool m_PendingResize = false; 
		int m_PendingWidth = 0, m_PendingHeight = 0;
	};
}