/**
 * @file
 * @ingroup core
 * @brief Describes the camera matrices and occlusion policy for one render view.
 *
 * View and Projection matrices are supplied separately from scene packet collection. OcclusionCulling enables the conservative CPU visibility option for this view.
 *
 * @par Usage
 * Construct a view for each camera or viewport. Independent views must keep independent visibility and LOD history; do not reuse one camera result as another camera result.
 */
#pragma once
#include <glm/glm.hpp>

namespace FINALITY
{
    /**
     * @ingroup core
     * @brief Describes the camera matrices and occlusion policy for one render view.
     *
     * View and Projection matrices are supplied separately from scene packet collection. OcclusionCulling enables the conservative CPU visibility option for this view.
     *
     * @par Usage
     * Construct a view for each camera or viewport. Independent views must keep independent visibility and LOD history; do not reuse one camera result as another camera result.
     */
    struct RenderView
    {
        glm::mat4 View{ 1.0f };
        glm::mat4 Projection{ 1.0f };
        bool OcclusionCulling = false;
    };
} // namespace FINALITY
