#pragma once
#include <chrono>
#include <Finality.h>

namespace FINALITY 
{
	static void DrawPerformancePanel(float uiFps, int historyOffset, const float* frameTimeHistory, float uiAvgFrameTimeMs)
	{
		FINALITY::ImGUIPanel perfPanel("Performance Overlay");
		perfPanel.Text("System Benchmarks");
		perfPanel.Text("FPS: %.1f", uiFps);

		const int latestIndex = (historyOffset + 119) % 120;

		perfPanel.Text(
			"Frame Time: %.2f ms",
			frameTimeHistory[latestIndex]
		);

		perfPanel.Text(
			"Avg (1s): %.2f ms",
			uiAvgFrameTimeMs
		);

		perfPanel.Text(
			"GPU main render: %.2f ms",
			static_cast<float>(FINALITY::Renderer::GetDevice()->GetGPUTimeMS())
		);

		perfPanel.PlotLines(
			"##TimeGraph",
			frameTimeHistory,
			120,
			historyOffset,
			"Frame History (ms)",
			0.0f,
			33.3f,
			glm::vec2(200.0f, 60.0f)
		);
	}
}