#pragma once
#include <Core/Core.h>
#include <Core/RenderTypes.h>
#include <Core/RenderDevice.h>
#include <memory>

#include <string>

#include <Assets/Asset.h>

namespace FINALITY
{
    class FAPI Shader : public Asset
    {
    public:
        virtual ~Shader() = default;
        static std::unique_ptr<Shader> Create(const std::string& filePath, RenderDevice* device);
        virtual void Load(const std::string& filePath) = 0;
        virtual RendererAPI GetAPI() const = 0;

        AssetType GetType() const override { return AssetType::Shader; }
    };
}