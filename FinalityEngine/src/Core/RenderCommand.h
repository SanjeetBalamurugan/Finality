#pragma once
#include "RenderDevice.h"

namespace FINALITY
{
    class FAPI RenderCommand
    {
    public:
        inline static void SetClearColor(float r, float g, float b, float a)
        {
            s_RenderDevice->Clear(r, g, b, a);
        }

        inline static void Init(RenderDevice* device)
        {
            s_RenderDevice = device;
        }

    private:
        static RenderDevice* s_RenderDevice;
    };
}