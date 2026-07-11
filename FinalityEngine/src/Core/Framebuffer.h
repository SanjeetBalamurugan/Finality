#pragma once
#include <memory>
#include <Core/Core.h>

namespace FINALITY
{
    struct FramebufferSpecification
    {
        uint32_t Width = 1280;
        uint32_t Height = 720;
        bool IsSwapChainTarget = false;
    };

    class FAPI Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual const FramebufferSpecification& GetSpecification() const = 0;
        virtual void* GetColorAttachmentRendererID() const = 0;

        static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
    };
}
