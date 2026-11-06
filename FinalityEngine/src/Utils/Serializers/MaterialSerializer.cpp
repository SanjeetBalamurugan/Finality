#include "MaterialSerializer.h"
#include <Core/Log.h>
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <set>
#include <stdexcept>

namespace
{
    using namespace FINALITY;

    const char* ToString(MaterialParameterType type)
    {
        switch (type)
        {
        case MaterialParameterType::Float: return "Float";
        case MaterialParameterType::Vec4: return "Vec4";
        case MaterialParameterType::Mat4: return "Mat4";
        }
        return "Float";
    }

    MaterialParameterType ParseType(const std::string& value)
    {
        if (value == "Float") return MaterialParameterType::Float;
        if (value == "Vec4") return MaterialParameterType::Vec4;
        if (value == "Mat4") return MaterialParameterType::Mat4;
        throw std::runtime_error("Unsupported material parameter type: " + value);
    }

    size_t FloatCount(MaterialParameterType type)
    {
        switch (type)
        {
        case MaterialParameterType::Float: return 1;
        case MaterialParameterType::Vec4: return 4;
        case MaterialParameterType::Mat4: return 16;
        }
        return 0;
    }

    const char* CullToString(CullMode mode)
    {
        switch (mode)
        {
        case CullMode::None: return "None";
        case CullMode::Front: return "Front";
        case CullMode::Back: return "Back";
        }
        return "Back";
    }

    CullMode ParseCull(const std::string& mode)
    {
        if (mode == "None") return CullMode::None;
        if (mode == "Front") return CullMode::Front;
        if (mode == "Back") return CullMode::Back;
        throw std::runtime_error("Invalid culling mode: " + mode);
    }

    void Validate(const MaterialDefinition& definition)
    {
        if (definition.VertexShader.empty() || definition.FragmentShader.empty())
            throw std::runtime_error("Both vertex and fragment shader paths are required");
        if (definition.Type.empty())
            throw std::runtime_error("Material type must not be empty");

        std::set<std::string> names;
        std::array<bool, Material::PushConstantBytes> occupied{};
        for (const auto& param : definition.Properties)
        {
            if (param.Name.empty() || !names.insert(param.Name).second)
                throw std::runtime_error("Missing or duplicate property name: " + param.Name);

            const auto size = static_cast<uint32_t>(FloatCount(param.Type) * sizeof(float));
            if (param.Offset < Material::SerializedMaterialStart ||
                param.Offset > Material::PushConstantBytes ||
                size > Material::PushConstantBytes - param.Offset)
                throw std::runtime_error("Property out of material push-constant range: " + param.Name);

            // Explicit offset is deliberate: must agree with layout(offset=N) in GLSL.
            if (param.Offset % (param.Type == MaterialParameterType::Float ? 4u : 16u) != 0)
                throw std::runtime_error("Property has invalid alignment: " + param.Name);

            for (uint32_t byte = param.Offset; byte < param.Offset + size; ++byte)
            {
                if (occupied[byte])
                    throw std::runtime_error("Overlapping material property: " + param.Name);
                occupied[byte] = true;
            }
        }

        if (!definition.TexturePath.empty() && definition.TextureBinding.empty())
            throw std::runtime_error("Texture path requires a texture binding name");
    }
}

bool FINALITY::MaterialSerializer::Deserialize(const std::string& filePath, MaterialDefinition& output)
{
    try
    {
        YAML::Node doc = YAML::LoadFile(filePath);
        const YAML::Node material = doc["Material"];
        if (!material || !material.IsMap())
            throw std::runtime_error("Missing Material root");
        if (!material["Version"] || material["Version"].as<int>() != 1)
            throw std::runtime_error("Unsupported .fmat version (expected 1)");

        MaterialDefinition parsed;
        parsed.Name = material["Name"] ? material["Name"].as<std::string>() : "Untitled Material";
        parsed.Type = material["Type"] ? material["Type"].as<std::string>() : "Custom";

        const auto shaders = material["Shaders"];
        if (!shaders || !shaders.IsMap())
            throw std::runtime_error("Missing Shaders mapping");
        parsed.VertexShader = shaders["Vertex"].as<std::string>();
        parsed.FragmentShader = shaders["Fragment"].as<std::string>();

        const auto pipeline = material["Pipeline"];
        if (pipeline)
        {
            if (pipeline["Blending"])
                parsed.EnableBlending = pipeline["Blending"].as<bool>();
            if (pipeline["Culling"])
                parsed.Culling = ParseCull(pipeline["Culling"].as<std::string>());
        }

        const auto properties = material["Properties"];
        if (properties)
        {
            if (!properties.IsSequence())
                throw std::runtime_error("Properties must be a sequence");

            for (const auto& item : properties)
            {
                MaterialParameterDefinition property;
                property.Name = item["Name"].as<std::string>();
                property.Type = ParseType(item["Type"].as<std::string>());
                property.Offset = item["Offset"].as<uint32_t>();
                const auto value = item["Default"];
                const size_t count = FloatCount(property.Type);

                if (count == 1)
                {
                    if (value) property.Default[0] = value.as<float>();
                }
                else if (value)
                {
                    if (!value.IsSequence() || value.size() != count)
                        throw std::runtime_error("Wrong number of default values for " + property.Name);
                    for (size_t i = 0; i < count; ++i)
                        property.Default[i] = value[i].as<float>();
                }
                parsed.Properties.push_back(property);
            }
        }

        if (const auto texture = material["Texture"]; texture)
        {
            parsed.TextureBinding = texture["Binding"] ? texture["Binding"].as<std::string>() : "textureSampler";
            parsed.TexturePath = texture["Path"] ? texture["Path"].as<std::string>() : "";
        }

        Validate(parsed);
        output = std::move(parsed);
        return true;
    }
    catch (const std::exception& e)
    {
        FI_CORE_ERROR("Failed to deserialize .fmat '{}': {}", filePath, e.what());
        return false;
    }
}

bool FINALITY::MaterialSerializer::Serialize(const MaterialDefinition& definition, const std::string& filePath)
{
    try
    {
        Validate(definition);
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Version" << YAML::Value << 1;
        out << YAML::Key << "Name" << YAML::Value << definition.Name;
        out << YAML::Key << "Type" << YAML::Value << definition.Type;

        out << YAML::Key << "Shaders" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Vertex" << YAML::Value << definition.VertexShader;
        out << YAML::Key << "Fragment" << YAML::Value << definition.FragmentShader;
        out << YAML::EndMap;

        out << YAML::Key << "Pipeline" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Blending" << YAML::Value << definition.EnableBlending;
        out << YAML::Key << "Culling" << YAML::Value << CullToString(definition.Culling);
        out << YAML::EndMap;

        out << YAML::Key << "Properties" << YAML::Value << YAML::BeginSeq;
        for (const auto& property : definition.Properties)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Name" << YAML::Value << property.Name;
            out << YAML::Key << "Type" << YAML::Value << ToString(property.Type);
            out << YAML::Key << "Offset" << YAML::Value << property.Offset;
            out << YAML::Key << "Default" << YAML::Value;

            const auto count = FloatCount(property.Type);
            if (count == 1)
            {
                out << property.Default[0];
            }
            else
            {
                out << YAML::Flow << YAML::BeginSeq;
                for (size_t i = 0; i < count; ++i)
                    out << property.Default[i];
                out << YAML::EndSeq;
            }
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        if (!definition.TexturePath.empty())
        {
            out << YAML::Key << "Texture" << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "Binding" << YAML::Value << definition.TextureBinding;
            out << YAML::Key << "Path" << YAML::Value << definition.TexturePath;
            out << YAML::EndMap;
        }

        out << YAML::EndMap << YAML::EndMap;

        std::ofstream stream(filePath, std::ios::binary);
        if (!stream)
            throw std::runtime_error("Cannot open material file for writing");
        stream << out.c_str();
        if (!stream)
            throw std::runtime_error("Error while writing material file");
        return true;
    }
    catch (const std::exception& e)
    {
        FI_CORE_ERROR("Failed to serialize .fmat '{}': {}", filePath, e.what());
        return false;
    }
}
