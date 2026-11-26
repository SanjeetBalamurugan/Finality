/**
 * @file
 * @ingroup scene
 * @brief Stores authored level-of-detail thresholds and renderer membership.
 *
 * Each LODLevel names renderer entity UUIDs and a screen-relative height threshold. The group holds local bounds, automatic-bound controls and hysteresis; runtime visibility maintains independent selection history for each view.
 *
 * @par Usage
 * Assign renderer entities to levels, normalize the group after editing and recalculate bounds when needed. Duplication and prefab instantiation must remap stored renderer UUIDs.
 */
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <Core/Core.h>
#include <entt.hpp>

namespace FINALITY
{
    class Scene;

    /** @brief One screen-relative LOD threshold and its renderer entity UUIDs. */
    struct LODLevel
    {
        float ScreenRelativeHeight = 0.1f;
        std::vector<uint64_t> Renderers;
    };

    /**
     * @ingroup scene
     * @brief Stores authored level-of-detail thresholds and renderer membership.
     *
     * Each LODLevel names renderer entity UUIDs and a screen-relative height threshold. The group holds local bounds, automatic-bound controls and hysteresis; runtime visibility maintains independent selection history for each view.
     *
     * @par Usage
     * Assign renderer entities to levels, normalize the group after editing and recalculate bounds when needed. Duplication and prefab instantiation must remap stored renderer UUIDs.
     */
    struct LODGroupComponent
    {
        bool Enabled = true;
        bool AutoBounds = true;
        glm::vec3 LocalCenter{ 0 };
        float LocalSize = 1.0f;
        float Hysteresis = 0.05f;
        std::vector<LODLevel> Levels{ { 0.6f, {} }, { 0.3f, {} }, { 0.01f, {} } };
    };

    FAPI void NormalizeLODGroup(LODGroupComponent& group);
    FAPI bool RecalculateLODBounds(Scene& scene, entt::entity entity);
} // namespace FINALITY
