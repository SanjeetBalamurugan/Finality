#include "MaterialAsset.h"

#include <Assets/AssetManager.h>
#include <Core/Log.h>
#include <Core/RenderDevice.h>
#include <Renderer/Renderer.h>
#include <Renderer/ShaderCompiler.h>
#include <Renderer/ShaderReflection.h>
#include <Utils/Serializers/MaterialSerializer.h>

#include <filesystem>
#include <stdexcept>
#include <utility>
#include <algorithm>

namespace
{
    void ReflectProperties(FINALITY::MaterialDefinition& definition, const std::string& vertex, const std::string& fragment)
    {
        using namespace FINALITY;
        const std::string shaderFiles[] =
        {
            vertex,
            fragment
        };

        for (const auto& shader : shaderFiles)
        {
            for (const auto& uniform : ShaderReflection::Reflect(shader))
            {
                if (uniform.Storage == "Texture" && uniform.Set == 1 && uniform.Binding == 0 && definition.TextureBinding.empty())
                {
                    definition.TextureBinding = uniform.Name;
                }
                if (!uniform.Editable)
                {
                    continue;
                }
                const auto type = uniform.Type == "float" ? MaterialParameterType::Float : uniform.Type == "float4" ? MaterialParameterType::Vec4 : MaterialParameterType::Mat4;
                const auto existing = std::find_if(definition.Properties.begin(), definition.Properties.end(),
                                                   [&](const auto& property)
                                                   {
                                                       return property.Name == uniform.Name;
                                                   });
                if (existing != definition.Properties.end())
                {
                    if (existing->Type != type || existing->Offset != uniform.Offset)
                    {
                        throw std::runtime_error("Material field disagrees with shader layout: " + uniform.Name);
                    }
                    continue;
                }
                const bool overlaps = std::any_of(definition.Properties.begin(), definition.Properties.end(), [&](const auto& property)
                                                  {
                                                      const uint32_t size = property.Type == MaterialParameterType::Float ? 4 : property.Type == MaterialParameterType::Vec4 ? 16
                                                                                                                                                                             : 64;
                                                      return uniform.Offset < property.Offset + size && property.Offset < uniform.Offset + uniform.Size;
                                                  });
                if (overlaps)
                {
                    throw std::runtime_error("Shader field overlaps a declared material field: " + uniform.Name);
                }
                MaterialParameterDefinition property;
                property.Name = uniform.Name;
                property.Type = type;
                property.Offset = uniform.Offset;
                if (type == MaterialParameterType::Vec4)
                {
                    property.Default = { 1, 1, 1, 1 };
                }
                if (type == MaterialParameterType::Mat4)
                {
                    for (int diagonal = 0; diagonal < 4; ++diagonal)
                    {
                        property.Default[diagonal * 5] = 1;
                    }
                }
                definition.Properties.push_back(property);
            }
        }
    }

    std::string ResolvePath(const std::string& referencedPath, const std::string& materialPath)
    {
        namespace fs = std::filesystem;
        const auto available = [](const fs::path& value)
        {
            return fs::is_regular_file(value) || (FINALITY::ShaderCompiler::IsSourceShader(value.string()) &&
                                                  fs::is_regular_file(value.string() + ".spv"));
        };
        fs::path path(referencedPath);
        const fs::path projectPath(FINALITY::AssetRegistry::Resolve(referencedPath));
        if (!projectPath.empty() && available(projectPath))
        {
            return projectPath.string();
        }
        if (available(path))
        {
            return path.lexically_normal().string();
        }

        if (path.is_relative())
        {
            fs::path besideMaterial = fs::path(materialPath).parent_path() / path;
            if (available(besideMaterial))
            {
                return besideMaterial.lexically_normal().string();
            }
        }

        throw std::runtime_error("Missing material dependency: " + referencedPath);
    }
} // namespace

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
    {
        return nullptr;
    }

    try
    {
        PipelineConfig config;
        asset->m_VertexShaderPath = ResolvePath(asset->m_Definition.VertexShader, filePath);
        asset->m_FragmentShaderPath = ResolvePath(asset->m_Definition.FragmentShader, filePath);
        config.VertexShader = Shader::Create(asset->m_VertexShaderPath, device);
        config.FragmentShader = Shader::Create(asset->m_FragmentShaderPath, device);
        asset->m_Definition.VertexShader = AssetRegistry::Reference(asset->m_VertexShaderPath);
        asset->m_Definition.FragmentShader = AssetRegistry::Reference(asset->m_FragmentShaderPath);

        if (!config.VertexShader || !config.FragmentShader)
        {
            throw std::runtime_error("Unable to load one or more shaders");
        }
        ReflectProperties(asset->m_Definition, asset->m_VertexShaderPath, asset->m_FragmentShaderPath);

        config.EnableBlending = asset->m_Definition.EnableBlending;
        config.DepthWrite = !config.EnableBlending;
        config.Culling = asset->m_Definition.Culling;
        asset->m_Pipeline = Pipeline::Create(config);
        if (!asset->m_Pipeline)
        {
            throw std::runtime_error("Unable to create material pipeline");
        }
        const auto timestamp = [](const std::string& path)
        {
            return std::filesystem::last_write_time(std::filesystem::is_regular_file(path) ? path : path + ".spv");
        };
        asset->m_VertexTimestamp = timestamp(asset->m_VertexShaderPath);
        asset->m_FragmentTimestamp = timestamp(asset->m_FragmentShaderPath);

        if (!asset->m_Definition.TexturePath.empty())
        {
            asset->m_Texture = AssetManager::CreateAsset<Texture>(
                ResolvePath(asset->m_Definition.TexturePath, filePath));
            if (!asset->m_Texture)
            {
                throw std::runtime_error("Unable to load material texture");
            }
        }
        else if (!asset->m_Definition.TextureBinding.empty())
        {
            asset->m_Texture = AssetManager::CreateAsset<Texture>("builtin://white");
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
    {
        return nullptr;
    }

    auto instance = std::make_shared<Material>(m_Pipeline);
    for (const auto& property : m_Definition.Properties)
    {
        uint32_t bytes = 0;
        switch (property.Type)
        {
        case MaterialParameterType::Float:
            bytes = sizeof(float);
            break;
        case MaterialParameterType::Vec4:
            bytes = sizeof(glm::vec4);
            break;
        case MaterialParameterType::Mat4:
            bytes = sizeof(glm::mat4);
            break;
        }
        if (!instance->RegisterUniform(property.Name, property.Offset, bytes))
        {
            return nullptr;
        }

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
            {
                for (size_t row = 0; row < 4; ++row)
                {
                    value[col][row] = property.Default[col * 4 + row];
                }
            }
            instance->SetUniformMat4(property.Name, value);
        }
    }

    if (m_Texture)
    {
        instance->SetTexture(m_Definition.TextureBinding, m_Texture);
    }

    m_Instances.push_back(instance);
    return instance;
}

bool FINALITY::MaterialAsset::ReloadShadersIfChanged(bool force)
{
    namespace fs = std::filesystem;
    try
    {
        if (!fs::exists(m_VertexShaderPath) || !fs::exists(m_FragmentShaderPath))
        {
            return false;
        }
        const auto vert = fs::last_write_time(m_VertexShaderPath);
        const auto frag = fs::last_write_time(m_FragmentShaderPath);
        if (!force && vert == m_VertexTimestamp && frag == m_FragmentTimestamp)
        {
            return false;
        }

        RenderDevice* device = Renderer::GetDevice();
        if (!device)
        {
            return false;
        }
        if (force && (!ShaderCompiler::EnsureCompiled(m_VertexShaderPath, true) || !ShaderCompiler::EnsureCompiled(m_FragmentShaderPath, true)))
        {
            return false;
        }
        PipelineConfig config;
        config.VertexShader = Shader::Create(m_VertexShaderPath, device);
        config.FragmentShader = Shader::Create(m_FragmentShaderPath, device);
        if (!config.VertexShader || !config.FragmentShader)
        {
            return false;
        }
        auto definition = m_Definition;
        ReflectProperties(definition, m_VertexShaderPath, m_FragmentShaderPath);
        config.EnableBlending = m_Definition.EnableBlending;
        config.DepthWrite = !config.EnableBlending;
        config.Culling = m_Definition.Culling;

        auto newPipeline = Pipeline::Create(config);
        if (!newPipeline)
        {
            return false;
        }
        std::vector<std::pair<std::shared_ptr<Material>, std::shared_ptr<Material>>> updated;
        for (const auto& weak : m_Instances)
        {
            const auto instance = weak.lock();
            if (!instance)
            {
                continue;
            }
            auto candidate = instance->Clone();
            for (const auto& property : definition.Properties)
            {
                if (candidate->GetUniforms().contains(property.Name))
                {
                    continue;
                }
                const uint32_t size = property.Type == MaterialParameterType::Float ? 4 : property.Type == MaterialParameterType::Vec4 ? 16
                                                                                                                                       : 64;
                if (!candidate->RegisterUniform(property.Name, property.Offset, size))
                {
                    throw std::runtime_error("Shader uniform cannot be added: " + property.Name);
                }
                if (property.Type == MaterialParameterType::Float)
                {
                    candidate->SetUniform1f(property.Name, property.Default[0]);
                }
                else if (property.Type == MaterialParameterType::Vec4)
                {
                    candidate->SetUniformVec4(property.Name, { property.Default[0], property.Default[1], property.Default[2], property.Default[3] });
                }
                else
                {
                    glm::mat4 value(1);
                    for (int column = 0; column < 4; ++column)
                    {
                        for (int row = 0; row < 4; ++row)
                        {
                            value[column][row] = property.Default[column * 4 + row];
                        }
                    }
                    candidate->SetUniformMat4(property.Name, value);
                }
            }
            updated.emplace_back(instance, std::move(candidate));
        }
        device->WaitIdle();
        m_Definition = std::move(definition);
        m_Pipeline = std::move(newPipeline);
        for (const auto& [instance, candidate] : updated)
        {
            *instance = *candidate;
        }
        for (auto it = m_Instances.begin(); it != m_Instances.end();)
        {
            if (auto instance = it->lock())
            {
                instance->SetPipeline(m_Pipeline);
                ++it;
            }
            else
            {
                it = m_Instances.erase(it);
            }
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
            {
                property.Default[0] = value;
            }
        }
        else if (property.Type == MaterialParameterType::Vec4)
        {
            glm::vec4 value(0.0f);
            if (instance.TryGetUniformVec4(property.Name, value))
            {
                for (size_t i = 0; i < 4; ++i)
                {
                    property.Default[i] = value[static_cast<int>(i)];
                }
            }
        }
        else if (property.Type == MaterialParameterType::Mat4)
        {
            glm::mat4 value(1.0f);
            if (instance.TryGetUniformMat4(property.Name, value))
            {
                for (size_t col = 0; col < 4; ++col)
                {
                    for (size_t row = 0; row < 4; ++row)
                    {
                        property.Default[col * 4 + row] = value[col][row];
                    }
                }
            }
        }
    }
}

void FINALITY::MaterialAsset::ReplaceTexture(const std::shared_ptr<Texture>& previous, const std::shared_ptr<Texture>& replacement)
{
    if (!previous || !replacement)
    {
        return;
    }
    if (m_Texture == previous)
    {
        m_Texture = replacement;
    }
    std::erase_if(m_Instances, [&](const std::weak_ptr<Material>& weak)
                  {
                      const auto instance = weak.lock();
                      if (!instance)
                      {
                          return true;
                      }
                      auto textures = instance->GetTextures();
                      for (const auto& [name, texture] : textures)
                      {
                          if (texture == previous)
                          {
                              instance->SetTexture(name, replacement);
                          }
                      }
                      return false;
                  });
}
