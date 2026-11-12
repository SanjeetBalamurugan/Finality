#pragma once
#include "Core.h"

#include <string>
#include <vector>
#include <utility>

namespace FINALITY
{
	struct WindowSpec
	{
		std::string title;
		int width;
		int height;
		bool isFullscreen;
	};

	struct NativeWindowHandle
	{
		void* WindowHandle;		// HWND (Win32), ANativeWindow* (Android), NSWindow*/UIWindow* (Apple) 
		void* DeviceContext;	// HINSTANCE (Win32), null for others
	};

	class FAPI Window
	{
	public:
		virtual ~Window() = default;

		virtual void Initialize(const WindowSpec& specification) = 0;
		virtual void Update() = 0;
		virtual void Shutdown() = 0;

		virtual bool ShouldClose() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetCallbacks() = 0;
		virtual bool ConsumePendingResize(int& outW, int& outH) = 0;

		virtual void SetCursorMode(bool hiddenAndLocked) = 0;
		virtual void SetWindowTitle(const std::string& title) = 0;

		virtual WindowSpec GetSpecifications() const = 0;
		virtual NativeWindowHandle GetNativeHandles() const = 0;
        void QueueDroppedPath(const std::string& path) { m_DroppedPaths.push_back(path); }
        std::vector<std::string> ConsumeDroppedPaths() { return std::exchange(m_DroppedPaths, {}); }
    private:
        std::vector<std::string> m_DroppedPaths;
	};
}
