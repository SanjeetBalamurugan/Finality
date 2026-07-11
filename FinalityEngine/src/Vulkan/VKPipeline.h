#pragma once

#include <Core/Pipeline.h>
#include "VKCore.h"

#include <string>
#include <vector>

namespace FINALITY
{
    class VKPipeline : public Pipeline
    {
    public:
        VKPipeline(VkDevice device, VkRenderPass renderPass, const PipelineConfig& config,
            VkDescriptorSetLayout globalLayout, VkDescriptorSetLayout materialLayout);

        VKPipeline(VkDevice device, VkRenderPass renderPass, const PipelineConfig& config,
            VkDescriptorSetLayout postProcessLayout);

        ~VKPipeline() override;

        VKPipeline(const VKPipeline&) = delete;
        VKPipeline& operator=(const VKPipeline&) = delete;

        void Bind(VkCommandBuffer commandBuffer);

        VkPipeline GetVKPipeline() const { return m_GraphicsPipeline; }
        VkPipelineLayout GetVKLayout() const { return m_PipelineLayout; }

    private:
        VkDevice m_Device = nullptr;
        VkPipelineLayout m_PipelineLayout = nullptr;
        VkPipeline m_GraphicsPipeline = nullptr;
    };
}
