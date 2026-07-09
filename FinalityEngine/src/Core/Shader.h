#pragma once
#include <Core/Core.h>
#include "RenderTypes.h"
#include "RenderDevice.h"
#include <memory>

#include <string>

namespace FINALITY
{
    class Shader
    {
    public:
        virtual ~Shader() = default;
        static std::unique_ptr<Shader> Create(RenderDevice* device, const std::string& filePath);
        virtual void Load(const std::string& filePath) = 0;
        virtual RendererAPI GetAPI() const = 0;
    };
}