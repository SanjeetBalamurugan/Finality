#pragma once
#include "Core.h"
#include "Window.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace FINALITY
{
	class Mesh;
	class Pipeline;
	struct PipelineConfig;
	struct Vertex;

	struct RenderPacket
	{
		std::shared_ptr<Mesh> MeshData;
		std::shared_ptr<Pipeline> PipelineInstance;
		glm::mat4 Transform{ 1.0f };

		std::vector<uint8_t> CustomPushData;
	};

	class FAPI RenderDevice
	{
	public:
		virtual ~RenderDevice() = default;

		virtual void Initialize(const NativeWindowHandle& handle) = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void PresentFrame() = 0;

		virtual void WaitIdle() = 0;
		virtual void Clear(float r, float g, float b, float a) = 0;

		virtual void SetWindowSpec(const WindowSpec& spec) = 0;

		virtual void DrawQueue(const std::vector<RenderPacket>& queue) = 0;

		virtual std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices) = 0;
		virtual std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) = 0;
		virtual std::shared_ptr<Pipeline> CreatePipeline(const PipelineConfig& config) = 0;
	};
}
