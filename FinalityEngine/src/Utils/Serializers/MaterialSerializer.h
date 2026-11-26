/**
 * @file
 * @ingroup tools
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for MaterialSerializer.h.
 */
#pragma once

#include <Core/Core.h>
#include <Assets/MaterialAsset.h>
#include <string>

namespace FINALITY
{
    /** @todo TODO: Document MaterialSerializer class semantics, ownership and usage. */
    class FAPI MaterialSerializer
    {
    public:
        static bool Deserialize(const std::string& filePath, MaterialDefinition& output);
        static bool Serialize(const MaterialDefinition& definition, const std::string& filePath);
    };
} // namespace FINALITY
