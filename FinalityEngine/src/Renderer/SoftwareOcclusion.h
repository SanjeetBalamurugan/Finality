/**
 * @file
 * @ingroup render
 * @brief Filters packets using conservative current-frame CPU occlusion.
 *
 * ApplySoftwareOcclusion rasterizes marked opaque occluder triangles into a bounded low-resolution depth buffer. Incomplete coverage, clipping and exhausted work budgets retain visibility rather than hiding uncertain geometry.
 *
 * @par Usage
 * Enable through the camera/view policy and mark only suitable opaque geometry as occluders. Measure OcclusionMS against avoided work before enabling broadly; this is not GPU Hi-Z.
 */
#pragma once
#include <Core/RenderDevice.h>
#include "RenderStatistics.h"

namespace FINALITY
{
    void ApplySoftwareOcclusion(std::vector<RenderPacket>& packets, const RenderView& view, RenderStatistics& statistics);
}
