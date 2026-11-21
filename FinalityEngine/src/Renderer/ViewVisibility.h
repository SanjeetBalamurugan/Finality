/**
 * @file
 * @ingroup render
 * @brief Separates per-view visibility selection from packet collection.
 *
 * Visibility helpers operate on submitted packets using a view and its state. This keeps camera-specific culling and LOD history independent from the source scene traversal.
 *
 * @par Usage
 * Use one visibility state per independently rendered view. Reset state when the owning view or its relevant scene data is replaced.
 */
#pragma once
#include <Core/RenderDevice.h>
#include "RenderStatistics.h"

namespace FINALITY
{
    FAPI std::vector<RenderPacket> BuildViewQueue(const std::vector<RenderPacket>& candidates, const RenderView& view, RenderStatistics& stats, ViewVisibilityState* state = nullptr);
}
