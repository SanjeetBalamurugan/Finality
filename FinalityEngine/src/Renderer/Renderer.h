#pragma once

#include <Core/Core.h>
#include <Core/RenderDevice.h>
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include <vector>
#include "Camera.h"

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

        static void SetActiveCamera(Camera* camera) { s_ActiveCamera = camera; }
        static Camera* GetActiveCamera() { return s_ActiveCamera; }
    private:
        static RenderDevice* s_Device;
        static std::vector<RenderPacket> s_RenderQueue;
        static inline Camera* s_ActiveCamera = nullptr;
    };
}
