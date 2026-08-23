#pragma once
#include "Core.h"
#include "Window.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "Texture.h"
#include "RenderTypes.h"
#include "RenderTarget.h"

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
		std::unordered_map<std::string, std::shared_ptr<Texture>> Textures;
		const void* MaterialKey = nullptr;
	};

	struct InstancePayload {
		glm::mat4 Transform;
		uint8_t CustomData[64];
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
		virtual WindowSpec GetWindowSpec() = 0;

		virtual void DrawQueue(const std::vector<RenderPacket>& queue) = 0;
		virtual RendererAPI GetActiveApi() const = 0;

		virtual std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices) = 0;
		virtual std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) = 0;
		virtual std::shared_ptr<Pipeline> CreatePipeline(const PipelineConfig& config) = 0;

		virtual std::shared_ptr<RenderTarget> CreateRenderTarget(uint32_t width, uint32_t height) = 0;
		virtual void ResizeRenderTarget(const std::shared_ptr<RenderTarget>& target, uint32_t width, uint32_t height) = 0;
		virtual void UpdateRenderTargetCamera(const std::shared_ptr<RenderTarget>& target, Camera* camera) = 0;
		virtual void DestroyRenderTargetGpuResources(RenderTarget & target) = 0;
		virtual void RenderSceneToTarget(const std::shared_ptr<RenderTarget>& target, const std::vector<RenderPacket>& queue) = 0;
		virtual void* GetRenderTargetTextureHandle(const std::shared_ptr<RenderTarget>& target) = 0;
		virtual void SetRenderTargetClearColor(const std::shared_ptr<RenderTarget>& target, float r, float g, float b, float a) = 0;
		virtual void SetGameRenderTarget(const std::shared_ptr<RenderTarget>& target) = 0;
		virtual void SetSwapchainClearColor(float r, float g, float b, float a) = 0;

		virtual void BeginTextureBatch() = 0;
		virtual void EndAndSubmitTextureBatch() = 0;
		virtual bool IsUploadBatchActive() const = 0;
	};
}
