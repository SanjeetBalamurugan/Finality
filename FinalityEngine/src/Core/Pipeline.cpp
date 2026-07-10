#include "Core/Pipeline.h"
#include "Core/Application.h"
#include "Renderer/Renderer.h"

namespace FINALITY
{
    std::shared_ptr<Pipeline> Pipeline::Create(const PipelineConfig& config)
    {
        switch (Application::GetActiveAPI())
        {
        case RendererAPI::VULKAN:
            return Renderer::GetDevice()->CreatePipeline(config);
        default:
            return nullptr;
        }
    }
}
