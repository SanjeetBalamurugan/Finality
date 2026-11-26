/**
 * @file
 * @ingroup core
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Pipeline.h.
 */
#pragma once

#include <memory>
#include "Renderer/Shader.h"

#include <Core/Core.h>

namespace FINALITY
{
    /** @todo TODO: Document CullMode enum semantics, ownership and usage. */
    enum class CullMode
    {
        None,
        Back,
        Front
    };

    /** @todo TODO: Document PipelineConfig struct semantics, ownership and usage. */
    struct PipelineConfig
    {
        std::shared_ptr<Shader> VertexShader;
        std::shared_ptr<Shader> FragmentShader;
        bool EnableBlending = false;
        bool DepthTest = true;
        bool DepthWrite = true;

        CullMode Culling = CullMode::Back;
    };

    /** @todo TODO: Document Pipeline class semantics, ownership and usage. */
    class FAPI Pipeline
    {
    public:
        virtual ~Pipeline() = default;
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;
        Pipeline() = default;

        static std::shared_ptr<Pipeline> Create(const PipelineConfig& config);

        bool IsTransparent() const
        {
            return m_Transparent;
        }

        bool WritesDepth() const
        {
            return m_DepthWrite;
        }

        bool TestsDepth() const
        {
            return m_DepthTest;
        }

        CullMode GetCullMode() const
        {
            return m_Culling;
        }

    protected:
        bool m_Transparent = false;
        bool m_DepthWrite = true;
        bool m_DepthTest = true;
        CullMode m_Culling = CullMode::Back;
    };
} // namespace FINALITY
