#pragma once

#include <Assets/Asset.h>
#include <Core/Core.h>
#include <Core/Pipeline.h>
#include <Renderer/Material.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace FINALITY
{
    class RenderDevice;

    enum class MaterialParameterType
    {
        Float,
        Vec4,
        Mat4
    };

    struct MaterialParameterDefinition
    {
        std::string Name;
        MaterialParameterType Type = MaterialParameterType::Float;
        uint32_t Offset = 64;
        // Float uses 1 element; Vec4 uses 4; Mat4 uses 16, column-major.
        std::array<float, 16> Default{};
    };

    struct MaterialDefinition
    {
        std::string Name = "Untitled Material";
        std::string Type = "Custom"; // Standard, Unlit, Custom or project-defined type
        std::string VertexShader;
        std::string FragmentShader;
        bool EnableBlending = false;
        CullMode Culling = CullMode::Back;
        std::vector<MaterialParameterDefinition> Properties;
        // Current Vulkan implementation binds one combined image sampler at set=1, binding=0.
        std::string TextureBinding = "textureSampler";
        std::string TexturePath;
    };

    // Cached shared asset loaded from a .fmat file. Do not animate this definition.
    class FAPI MaterialAsset : public Asset
    {
    public:
        static std::shared_ptr<MaterialAsset> Create(const std::string& filePath, RenderDevice* device);

        AssetType GetType() const override { return AssetType::Material; }
        const MaterialDefinition& GetDefinition() const { return m_Definition; }
        MaterialDefinition& GetDefinition() { return m_Definition; }

        std::shared_ptr<Material> CreateInstance() const;
        void UpdateDefaultsFromInstance(const Material& instance);
        bool ReloadShadersIfChanged(bool force = false);

    private:
        MaterialDefinition m_Definition;
        std::shared_ptr<Pipeline> m_Pipeline;
        std::shared_ptr<Texture> m_Texture;
        std::string m_FilePath;
        std::string m_VertexShaderPath;
        std::string m_FragmentShaderPath;
        std::filesystem::file_time_type m_VertexTimestamp{};
        std::filesystem::file_time_type m_FragmentTimestamp{};
        mutable std::vector<std::weak_ptr<Material>> m_Instances;
    };
}
