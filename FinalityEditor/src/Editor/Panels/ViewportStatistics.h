/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ViewportStatistics.h.
 */
#pragma once
#include <Core/RenderTarget.h>
#include <sstream>
#include <iomanip>
#include <imgui.h>

namespace FINALITY
{
    inline std::string FormatViewportStatistics(const RenderTarget& target)
    {
        const auto& s = target.Statistics;
        std::ostringstream text;
        text << std::fixed << std::setprecision(2);
        text << "Resolution: " << target.Width << " x " << target.Height << '\n';
        text << "Objects: " << s.Visible << " visible / " << s.Candidates << " candidates (" << s.Culled << " culled)\n";
        text << "Batches: " << s.DrawCalls << "  Instances: " << s.Instances << '\n';
        text << "Culled: frustum " << s.FrustumCulled << "  LOD " << s.LODCulled << "  occlusion " << s.OcclusionCulled << '\n';
        text << "LOD groups: " << s.LODGroups << "  transitions: " << s.LODTransitions << '\n';
        text << "CPU occlusion: " << s.OcclusionMS << " ms  occluder triangles: " << s.OccluderTriangles << '\n';
        text << "Triangles: " << s.Triangles << "  Vertices: " << s.Vertices << '\n';
        text << "Pipeline binds: " << s.PipelineBinds << "  Descriptor binds: " << s.DescriptorBinds << '\n';
        text << "CPU visibility: " << s.VisibilityMS << " ms\nCPU recording: " << s.RecordMS << " ms\nGPU view: ";
        if (s.GpuValid)
        {
            text << s.GpuMS << " ms (delayed sample)";
        }
        else
        {
            text << "pending / unsupported";
        }
        return text.str();
    }

    inline void DrawViewportStatsOverlay(const RenderTarget& target, const ImVec2& minimum, const ImVec2& maximum)
    {
        const auto text = FormatViewportStatistics(target);
        const auto size = ImGui::CalcTextSize(text.c_str());
        const ImVec2 origin{ std::max(minimum.x + 8, maximum.x - size.x - 20), minimum.y + 8 };
        auto* draw = ImGui::GetWindowDrawList();
        draw->PushClipRect(minimum, maximum, true);
        auto background = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
        background.w = 0.94f;
        draw->AddRectFilled(origin, { origin.x + size.x + 12, origin.y + size.y + 12 }, ImGui::GetColorU32(background), 4);
        draw->AddText({ origin.x + 6, origin.y + 6 }, ImGui::GetColorU32(ImGuiCol_Text), text.c_str());
        draw->PopClipRect();
    }
} // namespace FINALITY
