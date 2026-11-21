/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for PerformancePanel.h.
 */
#pragma once
#include <Finality.h>
#include "ViewportStatistics.h"
#include <algorithm>

namespace FINALITY
{
    inline void DrawPerformancePanel(float fps, int offset, const float* history, float average, const RenderTarget* scene, const RenderTarget* game, bool* open = nullptr)
    {
        if (!ImGui::Begin("Performance Overlay",open)) { ImGui::End(); return; }
        ImGui::Text("FPS: %.1f | Frame average: %.2f ms",fps,average);
        std::vector<float> samples;
        for (int i = 0; i < 120; ++i) if (history[i] > 0) samples.push_back(history[i]);
        if (!samples.empty())
        {
            std::sort(samples.begin(),samples.end());
            auto percentile = [&](float fraction) { return samples[static_cast<size_t>(fraction*(samples.size()-1))]; };
            ImGui::Text("CPU wall frame p50 / p95 / p99: %.2f / %.2f / %.2f ms",percentile(0.5f),percentile(0.95f),percentile(0.99f));
        }
        ImGui::Text("GPU main/postprocess only: %.2f ms",Renderer::GetDevice()->GetGPUTimeMS());
        if (const auto* active = SceneManager::GetInstance().GetActiveScene())
            ImGui::Text("Script update + cleanup CPU: %.3f ms | Calls: %llu",SceneManager::GetInstance().IsSimulationEnabled() ? active->GetScriptTimeMS() : 0,
                static_cast<unsigned long long>(SceneManager::GetInstance().IsSimulationEnabled() ? active->GetScriptCalls() : 0));
        if (scene && ImGui::CollapsingHeader("Scene View",ImGuiTreeNodeFlags_DefaultOpen)) ImGui::TextUnformatted(FormatViewportStatistics(*scene).c_str());
        if (game && ImGui::CollapsingHeader("Game View",ImGuiTreeNodeFlags_DefaultOpen)) ImGui::TextUnformatted(FormatViewportStatistics(*game).c_str());
        ImGui::PlotLines("##FrameHistory",history,120,offset,"Frame history (ms)",0,50,{0,80});
        ImGui::TextDisabled("GPU viewport measurements are delayed until their frame fence completes.");
        ImGui::End();
    }
}
