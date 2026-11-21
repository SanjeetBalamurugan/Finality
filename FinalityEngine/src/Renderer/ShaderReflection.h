/**
 * @file
 * @ingroup render
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ShaderReflection.h.
 */
#pragma once

#include <Core/Core.h>
#include <cstdint>
#include <string>
#include <vector>

namespace FINALITY
{
/** @todo TODO: Document ShaderUniform struct semantics, ownership and usage. */
struct ShaderUniform
{
    std::string Name;
    std::string Type;
    std::string Block;
    std::string Storage;
    uint32_t Offset = 0;
    uint32_t Size = 0;
    uint32_t Set = 0;
    uint32_t Binding = 0;
    bool Editable = false;
};

/** @todo TODO: Document ShaderReflection class semantics, ownership and usage. */
class FAPI ShaderReflection
{
  public:
    static std::vector<ShaderUniform> Reflect(const std::string &source);
};
}
