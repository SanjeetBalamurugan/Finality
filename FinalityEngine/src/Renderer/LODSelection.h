/**
 * @file
 * @ingroup render
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for LODSelection.h.
 */
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace FINALITY
{
    /** @todo TODO: Document RenderLODGroup struct semantics, ownership and usage. */
    struct RenderLODGroup
    {
        uint64_t ID = 0;
        glm::vec3 Center{ 0 };
        float Size = 1;
        float Hysteresis = 0.05f;
        std::vector<float> Thresholds;
    };

    /** @todo TODO: Document ViewVisibilityState struct semantics, ownership and usage. */
    struct ViewVisibilityState
    {
        std::unordered_map<uint64_t, int> LODLevels;
    };
} // namespace FINALITY
