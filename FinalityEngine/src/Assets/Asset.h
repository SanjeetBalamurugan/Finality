#pragma once
#include <Utils/UUID.h>
#include <string>

namespace FINALITY
{
    enum class AssetType : uint16_t
    {
        None = 0,
        Texture,
        Shader,
        Mesh,
        Pipeline
    };

    class Asset
    {
    public:
        UUID Handle;

        virtual ~Asset() = default;
        virtual AssetType GetType() const = 0;
    };
}
