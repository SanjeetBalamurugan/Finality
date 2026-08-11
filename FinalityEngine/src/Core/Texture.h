#pragma once
#include <string>
#include <memory>
#include <Core/Core.h>
#include <Assets/Asset.h>

namespace FINALITY
{
    class FAPI Texture : public Asset
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetMipLevels() const = 0;

        virtual void Bind(uint32_t slot = 0) const = 0;

        AssetType GetType() const override { return AssetType::Texture; }

        static std::shared_ptr<Texture> Create(const std::string& filePath);

    };
}
