#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstring>
#include <glm/glm.hpp>
#include <Core/Pipeline.h>
#include <Core/Texture.h>

#include <Core/Core.h>

namespace FINALITY
{
    struct UniformMember
    {
        uint32_t Offset;
        uint32_t Size;
    };

    class FAPI Material
    {
    public:
        Material(std::shared_ptr<Pipeline> pipeline)
            : m_Pipeline(pipeline)
        {
            m_DataBuffer.resize(128, 0);
        }

        virtual ~Material() = default;

        void RegisterUniform(const std::string& name, uint32_t offset, uint32_t size)
        {
            m_Uniforms[name] = { offset, size };
        }

        void SetUniform1f(const std::string& name, float value)
        {
            auto it = m_Uniforms.find(name);
            if (it != m_Uniforms.end() && it->second.Size == sizeof(float))
            {
                std::memcpy(m_DataBuffer.data() + it->second.Offset, &value, sizeof(float));
            }
        }

        void SetUniformMat4(const std::string& name, const glm::mat4& value)
        {
            auto it = m_Uniforms.find(name);
            if (it != m_Uniforms.end() && it->second.Size == sizeof(glm::mat4))
            {
                std::memcpy(m_DataBuffer.data() + it->second.Offset, &value, sizeof(glm::mat4));
            }
        }

        void SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
        {
            m_Textures[name] = texture;
            m_MaterialDirty = true;
        }

        bool IsDirty() const { return m_MaterialDirty; }
        void ClearDirtyFlag() { m_MaterialDirty = false; }

        const std::vector<uint8_t>& GetRawDataBuffer() const { return m_DataBuffer; }
        std::shared_ptr<Pipeline> GetPipeline() const { return m_Pipeline; }
        std::unordered_map<std::string, std::shared_ptr<Texture>>& GetTextures() { return m_Textures; }

    private:
        std::shared_ptr<Pipeline> m_Pipeline;
        std::vector<uint8_t> m_DataBuffer;
        std::unordered_map<std::string, UniformMember> m_Uniforms;

        std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
        bool m_MaterialDirty = false;
    };
}
