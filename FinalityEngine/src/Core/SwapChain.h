#pragma once
#include "Window.h"

namespace FINALITY
{
    class SwapChain
    {
    public:
        virtual ~SwapChain() = default;

        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain() = default;

        virtual void Initialize(const NativeWindowHandle& handle, const WindowSpec& spec) = 0;
        virtual void Recreate(const WindowSpec& spec) = 0;
        virtual void Shutdown() = 0;

        virtual uint32_t AcquireNextImage() = 0;
        virtual void Present() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetImageCount() const = 0;
        virtual uint32_t GetCurrentImageIndex() const = 0;
    };
}