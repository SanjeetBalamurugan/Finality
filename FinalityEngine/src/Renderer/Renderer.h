#pragma once

#include <Core/Core.h>
#include <Core/RenderDevice.h>
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include <vector>

namespace FINALITY
{
    class FAPI Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        static void Initialize(RenderDevice* device);
        static void Shutdown();

        static void BeginScene();
        static void EndScene();

        static void PushEntity(Entity entity);
        static RenderDevice* GetDevice() { return s_Device; }
    private:
        static RenderDevice* s_Device;
        static std::vector<RenderPacket> s_RenderQueue;
    };
}
