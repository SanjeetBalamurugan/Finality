/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for GameViewSettings.h.
 */
#pragma once
#include <glm/glm.hpp>
#include <algorithm>

namespace FINALITY
{
    /** @todo TODO: Document GameViewSettings struct semantics, ownership and usage. */
    struct GameViewSettings
    {
        int Preset = 1;
        bool Stats = false;
        glm::vec2 Resolution(glm::vec2 available) const
        {
            if (Preset == 3) return {1280,720};
            if (Preset == 4) return {1920,1080};
            if (Preset == 5) return {2560,1440};
            const float aspect = Preset == 1 ? 16.0f/9 : Preset == 2 ? 4.0f/3 : available.x/std::max(available.y,1.0f);
            if (available.x/available.y > aspect) available.x = available.y*aspect;
            else available.y = available.x/aspect;
            return available;
        }
    };
}
