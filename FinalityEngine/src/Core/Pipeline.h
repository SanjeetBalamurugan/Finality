#pragma once

#include <memory>
#include "Renderer/Shader.h"

#include <Core/Core.h>

namespace FINALITY
{
    struct PipelineConfig
    {
        std::shared_ptr<Shader> VertexShader;
        std::shared_ptr<Shader> FragmentShader;
        bool EnableBlending = false;
    };

    class FAPI Pipeline
    {
    public:
        virtual ~Pipeline() = default;
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;
        Pipeline() = default;

        static std::shared_ptr<Pipeline> Create(const PipelineConfig& config);
    };
}
