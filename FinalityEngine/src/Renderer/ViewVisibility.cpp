#include "ViewVisibility.h"
#include <Core/Mesh.h>
#include "VisibilityMath.h"
#include "SoftwareOcclusion.h"
#include "Camera/Frustum.h"
#include <chrono>
#include <cmath>

namespace
{
    int SelectLOD(const FINALITY::RenderLODGroup& group, const FINALITY::RenderView& view, FINALITY::ViewVisibilityState& state, FINALITY::RenderStatistics& stats)
    {
        const auto position = view.View * glm::vec4(group.Center, 1);
        const float depth = -position.z;
        const bool perspective = std::abs(view.Projection[3][3]) < 0.00001f;
        const float height = perspective && depth <= group.Size * 0.5f ? 1000.0f : std::abs(view.Projection[1][1]) * group.Size / (perspective ? 2 * std::max(depth, 0.00001f) : 2);
        const int count = static_cast<int>(group.Thresholds.size());
        int selected = 0;
        while (selected < count && height < group.Thresholds[selected])
        {
            ++selected;
        }
        if (auto previous = state.LODLevels.find(group.ID); previous != state.LODLevels.end())
        {
            selected = std::clamp(previous->second, 0, count);
            while (selected > 0 && height >= group.Thresholds[selected - 1] * (1 + group.Hysteresis))
            {
                --selected;
            }
            while (selected < count && height < group.Thresholds[selected] * (1 - group.Hysteresis))
            {
                ++selected;
            }
            if (selected != previous->second)
            {
                ++stats.LODTransitions;
            }
        }
        return selected;
    }
} // namespace

std::vector<FINALITY::RenderPacket> FINALITY::BuildViewQueue(const std::vector<RenderPacket>& candidates, const RenderView& view, RenderStatistics& stats, ViewVisibilityState* state)
{
    const auto begin = std::chrono::steady_clock::now();
    stats.Candidates = stats.Visible = stats.Culled = 0;
    stats.FrustumCulled = stats.LODCulled = stats.OcclusionCulled = 0;
    stats.LODGroups = stats.LODTransitions = stats.OccluderTriangles = 0;
    stats.OcclusionMS = 0;
    ViewVisibilityState temporary;
    if (!state)
    {
        state = &temporary;
    }
    std::unordered_map<uint64_t, int> selected;
    const auto frustum = Frustum::FromViewProjection(view.Projection * view.View);
    std::vector<RenderPacket> visible;
    visible.reserve(candidates.size());
    for (const auto& packet : candidates)
    {
        if (!packet.MeshData || !packet.PipelineInstance)
        {
            continue;
        }
        if (!packet.EditorOverlay)
        {
            ++stats.Candidates;
            if (packet.LODGroup)
            {
                const auto& group = *packet.LODGroup;
                if (!selected.contains(group.ID))
                {
                    selected[group.ID] = SelectLOD(group, view, *state, stats);
                }
                if (packet.LODLevel != selected.at(group.ID))
                {
                    ++stats.LODCulled;
                    ++stats.Culled;
                    continue;
                }
            }
            const auto center = glm::vec3(packet.Transform * glm::vec4(packet.MeshData->GetBoundsCenter(), 1));
            const float radius = packet.MeshData->GetBoundsRadius() * ConservativeScale(packet.Transform);
            if (!frustum.IntersectsSphere(center, radius))
            {
                ++stats.FrustumCulled;
                ++stats.Culled;
                continue;
            }
            ++stats.Visible;
        }
        visible.push_back(packet);
    }
    stats.LODGroups = selected.size();
    state->LODLevels = std::move(selected);
    if (view.OcclusionCulling)
    {
        ApplySoftwareOcclusion(visible, view, stats);
    }
    stats.VisibilityMS = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    return visible;
}
