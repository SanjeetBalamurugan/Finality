/**
 * @file
 * @ingroup assets
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Asset.h.
 */
#pragma once
#include <Utils/UUID.h>
#include <string>
#include <Core/Core.h>

namespace FINALITY
{
    /** @todo TODO: Document AssetType enum semantics, ownership and usage. */
    enum class AssetType : uint16_t
    {
        None = 0,
        Texture,
        Shader,
        Mesh,
        Pipeline,
        Material
    };

    /** @todo TODO: Document Asset class semantics, ownership and usage. */
    class FAPI Asset
    {
    public:
        UUID Handle;

        virtual ~Asset() = default;
        virtual AssetType GetType() const = 0;
    };
} // namespace FINALITY
