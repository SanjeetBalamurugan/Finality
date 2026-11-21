/**
 * @file
 * @ingroup render
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Shader.h.
 */
#pragma once
#include <Core/Core.h>
#include <Core/RenderTypes.h>
#include <Core/RenderDevice.h>
#include <memory>

#include <string>

#include <Assets/Asset.h>

namespace FINALITY
{
    /** @todo TODO: Document Shader class semantics, ownership and usage. */
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