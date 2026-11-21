/**
 * @file
 * @ingroup render
 * @brief Reports visibility, submission and timing measurements.
 *
 * Counters separate frustum, LOD and occlusion rejections from draw calls, instances, triangles, vertices and binding changes. CPU visibility and recording times are distinct from GPU time.
 *
 * @par Usage
 * Read GpuMS only when GpuValid is true. Compare statistics for the same view and workload; script-only timing or one GPU pass does not establish whole-frame performance.
 */
#pragma once
#include <cstdint>

namespace FINALITY
{
    /**
     * @ingroup render
     * @brief Reports visibility, submission and timing measurements.
     *
     * Counters separate frustum, LOD and occlusion rejections from draw calls, instances, triangles, vertices and binding changes. CPU visibility and recording times are distinct from GPU time.
     *
     * @par Usage
     * Read GpuMS only when GpuValid is true. Compare statistics for the same view and workload; script-only timing or one GPU pass does not establish whole-frame performance.
     */
    struct RenderStatistics
    {
        uint64_t Candidates = 0, Visible = 0, Culled = 0;
        uint64_t FrustumCulled = 0, LODCulled = 0, OcclusionCulled = 0;
        uint64_t LODGroups = 0, LODTransitions = 0, OccluderTriangles = 0;
        double OcclusionMS = 0;
        uint64_t DrawCalls = 0, Instances = 0, Triangles = 0, Vertices = 0;
        uint64_t PipelineBinds = 0, DescriptorBinds = 0;
        double VisibilityMS = 0, RecordMS = 0, GpuMS = 0;
        bool GpuValid = false;
    };
}
