#pragma once

#include <Core/Core.h>
#include <Core/Pipeline.h>
#include <Core/Texture.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

namespace FINALITY
{
    struct UniformMember
    {
        uint32_t Offset = 0;
        uint32_t Size = 0;
    };

    class FAPI Material
    {
    public:
        static constexpr uint32_t PushConstantBytes = 128;
        static constexpr uint32_t SerializedMaterialStart = 64;

        explicit Material(std::shared_ptr<Pipeline> pipeline)
            : m_Pipeline(std::move(pipeline)), m_DataBuffer(PushConstantBytes, 0)
        {
        }

        virtual ~Material() = default;

        bool RegisterUniform(const std::string& name, uint32_t offset, uint32_t size)
        {
            if (name.empty() || size == 0 || offset > PushConstantBytes ||
                size > PushConstantBytes - offset)
                return false;

            for (const auto& [otherName, other] : m_Uniforms)
            {
                if (otherName == name)
                    continue;
                if (offset < other.Offset + other.Size &&
                    other.Offset < offset + size)
                    return false;
            }

            m_Uniforms[name] = { offset, size };
            return true;
        }

        bool SetUniform1f(const std::string& name, float value)
        {
            return WriteUniform(name, &value, sizeof(value));
        }

        bool SetUniformVec4(const std::string& name, const glm::vec4& value)
        {
            return WriteUniform(name, &value, sizeof(value));
        }

        bool SetUniformMat4(const std::string& name, const glm::mat4& value)
        {
            return WriteUniform(name, &value, sizeof(value));
        }

        bool TryGetUniform1f(const std::string& name, float& value) const
        {
            return ReadUniform(name, &value, sizeof(value));
        }

        bool TryGetUniformVec4(const std::string& name, glm::vec4& value) const
        {
            return ReadUniform(name, &value, sizeof(value));
        }

        bool TryGetUniformMat4(const std::string& name, glm::mat4& value) const
        {
            return ReadUniform(name, &value, sizeof(value));
        }

        void SetTexture(const std::string& name, std::shared_ptr<Texture> texture)
        {
            m_Textures[name] = std::move(texture);
            m_MaterialDirty = true;
        }

        bool IsDirty() const { return m_MaterialDirty; }
        void ClearDirtyFlag() { m_MaterialDirty = false; }

        const std::vector<uint8_t>& GetRawDataBuffer() const { return m_DataBuffer; }
        std::shared_ptr<Pipeline> GetPipeline() const { return m_Pipeline; }
        std::shared_ptr<Material> Clone() const
        {
            auto copy = std::make_shared<Material>(m_Pipeline);
            copy->m_DataBuffer = m_DataBuffer;
            copy->m_Uniforms = m_Uniforms;
            copy->m_Textures = m_Textures;
            copy->m_MaterialDirty = true;
            return copy;
        }
        const std::unordered_map<std::string, UniformMember>& GetUniforms() const { return m_Uniforms; }
        std::unordered_map<std::string, std::shared_ptr<Texture>>& GetTextures() { return m_Textures; }

        void SetPipeline(std::shared_ptr<Pipeline> pipeline)
        {
            m_Pipeline = std::move(pipeline);
            m_MaterialDirty = true;
        }


    private:
        bool WriteUniform(const std::string& name, const void* data, size_t size)
        {
            const auto it = m_Uniforms.find(name);
            if (it == m_Uniforms.end() || it->second.Size != size ||
                it->second.Offset > m_DataBuffer.size() ||
                size > m_DataBuffer.size() - it->second.Offset)
                return false;

            std::memcpy(m_DataBuffer.data() + it->second.Offset, data, size);
            m_MaterialDirty = true;
            return true;
        }

        bool ReadUniform(const std::string& name, void* data, size_t size) const
        {
            const auto it = m_Uniforms.find(name);
            if (it == m_Uniforms.end() || it->second.Size != size ||
                it->second.Offset > m_DataBuffer.size() ||
                size > m_DataBuffer.size() - it->second.Offset)
                return false;

            std::memcpy(data, m_DataBuffer.data() + it->second.Offset, size);
            return true;
        }

        std::shared_ptr<Pipeline> m_Pipeline;
        std::vector<uint8_t> m_DataBuffer;
        std::unordered_map<std::string, UniformMember> m_Uniforms;
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
        bool m_MaterialDirty = false;
    };
}
