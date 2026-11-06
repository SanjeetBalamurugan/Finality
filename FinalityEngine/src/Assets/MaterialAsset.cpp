#include "MaterialAsset.h"

#include <Assets/AssetManager.h>
#include <Core/Log.h>
#include <Core/RenderDevice.h>
#include <Renderer/Renderer.h>
#include <Renderer/ShaderCompiler.h>
#include <Utils/Serializers/MaterialSerializer.h>

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace
{
    std::string ResolvePath(const std::string& referencedPath, const std::string& materialPath)
    {
        namespace fs = std::filesystem;
        fs::path path(referencedPath);
        if (fs::exists(path))
            return path.lexically_normal().string();

        if (path.is_relative())
        {
            fs::path besideMaterial = fs::path(materialPath).parent_path() / path;
            if (fs::exists(besideMaterial))
                return besideMaterial.lexically_normal().string();
        }

        throw std::runtime_error("Missing material dependency: " + referencedPath);
    }
}

std::shared_ptr<FINALITY::MaterialAsset> FINALITY::MaterialAsset::Create(
    const std::string& filePath, RenderDevice* device)
{
    if (!device)
    {
        FI_CORE_ERROR("Cannot load material '{}' without a render device", filePath);
        return nullptr;
    }

    auto asset = std::shared_ptr<MaterialAsset>(new MaterialAsset());
    asset->m_FilePath = filePath;
    if (!MaterialSerializer::Deserialize(filePath, asset->m_Definition))
        return nullptr;

    try
    {
        PipelineConfig config;
        asset->m_VertexShaderPath = ResolvePath(asset->m_Definition.VertexShader, filePath);
        asset->m_FragmentShaderPath = ResolvePath(asset->m_Definition.FragmentShader, filePath);
        config.VertexShader = Shader::Create(asset->m_VertexShaderPath, device);
        config.FragmentShader = Shader::Create(asset->m_FragmentShaderPath, device);

        if (!config.VertexShader || !config.FragmentShader)
            throw std::runtime_error("Unable to load one or more shaders");

        config.EnableBlending = asset->m_Definition.EnableBlending;
        config.Culling = asset->m_Definition.Culling;
        asset->m_Pipeline = Pipeline::Create(config);
        if (!asset->m_Pipeline)
            throw std::runtime_error("Unable to create material pipeline");
        asset->m_VertexTimestamp = std::filesystem::last_write_time(asset->m_VertexShaderPath);
        asset->m_FragmentTimestamp = std::filesystem::last_write_time(asset->m_FragmentShaderPath);

        if (!asset->m_Definition.TexturePath.empty())
        {
            asset->m_Texture = AssetManager::CreateAsset<Texture>(
                ResolvePath(asset->m_Definition.TexturePath, filePath));
            if (!asset->m_Texture)
                throw std::runtime_error("Unable to load material texture");
        }

        return asset;
    }
    catch (const std::exception& e)
    {
        FI_CORE_ERROR("Failed to create material '{}': {}", filePath, e.what());
        return nullptr;
    }
}

std::shared_ptr<FINALITY::Material> FINALITY::MaterialAsset::CreateInstance() const
{
    if (!m_Pipeline)
        return nullptr;

    auto instance = std::make_shared<Material>(m_Pipeline);
    for (const auto& property : m_Definition.Properties)
    {
        uint32_t bytes = 0;
        switch (property.Type)
        {
        case MaterialParameterType::Float: bytes = sizeof(float); break;
        case MaterialParameterType::Vec4: bytes = sizeof(glm::vec4); break;
        case MaterialParameterType::Mat4: bytes = sizeof(glm::mat4); break;
        }
        if (!instance->RegisterUniform(property.Name, property.Offset, bytes))
            return nullptr;

        if (property.Type == MaterialParameterType::Float)
        {
            instance->SetUniform1f(property.Name, property.Default[0]);
        }
        else if (property.Type == MaterialParameterType::Vec4)
        {
            glm::vec4 value(property.Default[0], property.Default[1], property.Default[2], property.Default[3]);
            instance->SetUniformVec4(property.Name, value);
        }
        else if (property.Type == MaterialParameterType::Mat4)
        {
            glm::mat4 value(1.0f);
            for (size_t col = 0; col < 4; ++col)
                for (size_t row = 0; row < 4; ++row)
                    value[col][row] = property.Default[col * 4 + row];
            instance->SetUniformMat4(property.Name, value);
        }
    }

    if (m_Texture)
        instance->SetTexture(m_Definition.TextureBinding, m_Texture);

    m_Instances.push_back(instance);
    return instance;
}

bool FINALITY::MaterialAsset::ReloadShadersIfChanged(bool force)
{
    namespace fs = std::filesystem;
    try
    {
        if (!fs::exists(m_VertexShaderPath) || !fs::exists(m_FragmentShaderPath))
            return false;
        const auto vert = fs::last_write_time(m_VertexShaderPath);
        const auto frag = fs::last_write_time(m_FragmentShaderPath);
        if (!force && vert == m_VertexTimestamp && frag == m_FragmentTimestamp)
            return false;

        RenderDevice* device = Renderer::GetDevice();
        if (!device) return false;
        if (force && (!ShaderCompiler::EnsureCompiled(m_VertexShaderPath, true) ||
                      !ShaderCompiler::EnsureCompiled(m_FragmentShaderPath, true)))
            return false;
        PipelineConfig config;
        config.VertexShader = Shader::Create(m_VertexShaderPath, device);
        config.FragmentShader = Shader::Create(m_FragmentShaderPath, device);
        if (!config.VertexShader || !config.FragmentShader)
            return false;
        config.EnableBlending = m_Definition.EnableBlending;
        config.Culling = m_Definition.Culling;

        auto newPipeline = Pipeline::Create(config);
        if (!newPipeline) return false;
        device->WaitIdle();
        m_Pipeline = std::move(newPipeline);
        for (auto it = m_Instances.begin(); it != m_Instances.end(); )
        {
            if (auto instance = it->lock())
            {
                instance->SetPipeline(m_Pipeline);
                ++it;
            }
            else it = m_Instances.erase(it);
        }
        m_VertexTimestamp = vert;
        m_FragmentTimestamp = frag;
        FI_CORE_INFO("Rebuilt shaders for material '{}'", m_Definition.Name);
        return true;
    }
    catch (const std::exception& e)
    {
        FI_CORE_ERROR("Material shader reload failed: {}", e.what());
        return false;
    }
}

void FINALITY::MaterialAsset::UpdateDefaultsFromInstance(const Material& instance)
{
    for (auto& property : m_Definition.Properties)
    {
        if (property.Type == MaterialParameterType::Float)
        {
            float value = 0.0f;
            if (instance.TryGetUniform1f(property.Name, value))
                property.Default[0] = value;
        }
        else if (property.Type == MaterialParameterType::Vec4)
        {
            glm::vec4 value(0.0f);
            if (instance.TryGetUniformVec4(property.Name, value))
                for (size_t i = 0; i < 4; ++i)
                    property.Default[i] = value[static_cast<int>(i)];
        }
        else if (property.Type == MaterialParameterType::Mat4)
        {
            glm::mat4 value(1.0f);
            if (instance.TryGetUniformMat4(property.Name, value))
                for (size_t col = 0; col < 4; ++col)
                    for (size_t row = 0; row < 4; ++row)
                        property.Default[col * 4 + row] = value[col][row];
        }
    }
}
