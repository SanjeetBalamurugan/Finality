#pragma once

#include <Core/Core.h>
#include <Assets/MaterialAsset.h>
#include <string>

namespace FINALITY
{
    class FAPI MaterialSerializer
    {
    public:
        static bool Deserialize(const std::string& filePath, MaterialDefinition& output);
        static bool Serialize(const MaterialDefinition& definition, const std::string& filePath);
    };
}
