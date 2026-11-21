#include "ModelPipeline.h"
#include <Assets/AssetRegistry.h>
#include <Assets/MaterialAsset.h>
#include <Utils/Serializers/MaterialSerializer.h>
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <fstream>

void FINALITY::ModelPipeline::Save(const ModelData& model, const std::filesystem::path& directory, const std::string& signature)
{
    namespace fs = std::filesystem;
    fs::create_directories(directory);
    YAML::Emitter out;
    out << YAML::BeginMap << YAML::Key << "Version" << YAML::Value << 1 << YAML::Key << "Signature" << YAML::Value << signature;
    out << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;
    for (const auto& mesh : model.Meshes)
    {
        out << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << mesh.Name << YAML::Key << "Key" << YAML::Value << mesh.Key;
        out << YAML::Key << "Material" << YAML::Value << mesh.Material << YAML::EndMap;
        std::ofstream binary(directory / (mesh.Key + ".mesh"),std::ios::binary);
        const uint32_t header[] = {0x464D5348,1,sizeof(Vertex),static_cast<uint32_t>(mesh.Geometry.Vertices.size()),static_cast<uint32_t>(mesh.Geometry.Indices.size())};
        binary.write(reinterpret_cast<const char*>(header),sizeof(header));
        binary.write(reinterpret_cast<const char*>(mesh.Geometry.Vertices.data()),static_cast<std::streamsize>(mesh.Geometry.Vertices.size() * sizeof(Vertex)));
        binary.write(reinterpret_cast<const char*>(mesh.Geometry.Indices.data()),static_cast<std::streamsize>(mesh.Geometry.Indices.size() * sizeof(uint32_t)));
        if (!binary) throw std::runtime_error("Cannot write mesh cache");
    }
    out << YAML::EndSeq << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
    for (const auto& material : model.Materials)
    {
        out << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << material.Name << YAML::Key << "Asset" << YAML::Value << material.Asset;
        out << YAML::Key << "Texture" << YAML::Value << material.Texture << YAML::Key << "DoubleSided" << YAML::Value << material.DoubleSided;
        out << YAML::Key << "Color" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (int i = 0; i < 4; ++i) out << material.Color[i];
        out << YAML::EndSeq << YAML::Key << "SourceProperties" << YAML::Value << YAML::BeginMap;
        for (const auto& [key,value] : material.Properties) out << YAML::Key << key << YAML::Value << value;
        out << YAML::EndMap << YAML::EndMap;
    }
    out << YAML::EndSeq << YAML::Key << "Nodes" << YAML::Value << YAML::BeginSeq;
    for (const auto& node : model.Nodes)
    {
        out << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << node.Name << YAML::Key << "Parent" << YAML::Value << node.Parent;
        out << YAML::Key << "Transform" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (int column = 0; column < 4; ++column) for (int row = 0; row < 4; ++row) out << node.Transform[column][row];
        out << YAML::EndSeq << YAML::Key << "Meshes" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (auto mesh : node.Meshes) out << mesh;
        out << YAML::EndSeq << YAML::EndMap;
    }
    out << YAML::EndSeq << YAML::Key << "Bounds" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (int i = 0; i < 3; ++i) out << model.Minimum[i];
    for (int i = 0; i < 3; ++i) out << model.Maximum[i];
    out << YAML::EndSeq << YAML::Key << "Diagnostics" << YAML::Value << model.Diagnostics << YAML::Key << "Dependencies" << YAML::Value << model.Dependencies << YAML::EndMap;
    const auto temporary = directory / "Model.yaml.tmp";
    std::ofstream stream(temporary); stream << out.c_str(); stream.close();
    if (!stream) throw std::runtime_error("Cannot write model cache metadata");
    fs::copy_file(temporary,directory / "Model.yaml",fs::copy_options::overwrite_existing);
    fs::remove(temporary);
}

bool FINALITY::ModelPipeline::ReadCache(ModelData& model, const std::filesystem::path& directory, const std::string& signature)
{
    try
    {
        const auto root = YAML::LoadFile((directory / "Model.yaml").string());
        if (root["Version"].as<int>() != 1 || root["Signature"].as<std::string>() != signature) return false;
        ModelData parsed;
        for (const auto& item : root["Meshes"])
        {
            ModelMesh mesh;
            mesh.Name = item["Name"].as<std::string>(); mesh.Key = item["Key"].as<std::string>(); mesh.Material = item["Material"].as<uint32_t>();
            if (mesh.Key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos) return false;
            std::ifstream binary(directory / (mesh.Key + ".mesh"),std::ios::binary);
            uint32_t header[5]{}; binary.read(reinterpret_cast<char*>(header),sizeof(header));
            if (!binary || header[0] != 0x464D5348 || header[1] != 1 || header[2] != sizeof(Vertex) || header[3] > 10000000 || header[4] > 30000000) return false;
            mesh.Geometry.Vertices.resize(header[3]); mesh.Geometry.Indices.resize(header[4]);
            binary.read(reinterpret_cast<char*>(mesh.Geometry.Vertices.data()),static_cast<std::streamsize>(header[3]) * sizeof(Vertex));
            binary.read(reinterpret_cast<char*>(mesh.Geometry.Indices.data()),static_cast<std::streamsize>(header[4]) * sizeof(uint32_t));
            if (!binary) return false;
            for (auto index : mesh.Geometry.Indices) if (index >= mesh.Geometry.Vertices.size()) return false;
            parsed.Meshes.push_back(std::move(mesh));
        }
        for (const auto& item : root["Materials"])
        {
            ModelMaterial material;
            material.Name = item["Name"].as<std::string>(); material.Asset = item["Asset"].as<std::string>(); material.Texture = item["Texture"].as<std::string>(); material.DoubleSided = item["DoubleSided"].as<bool>();
            for (int i = 0; i < 4; ++i) material.Color[i] = item["Color"][i].as<float>();
            for (const auto& property : item["SourceProperties"]) material.Properties[property.first.as<std::string>()] = property.second.as<std::string>();
            parsed.Materials.push_back(std::move(material));
        }
        for (const auto& item : root["Nodes"])
        {
            ModelNode node;
            node.Name = item["Name"].as<std::string>(); node.Parent = item["Parent"].as<int>();
            if (node.Parent >= static_cast<int>(parsed.Nodes.size()) || node.Parent < -1 || item["Transform"].size() != 16) return false;
            for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) node.Transform[c][r] = item["Transform"][c * 4 + r].as<float>();
            node.Meshes = item["Meshes"].as<std::vector<uint32_t>>();
            for (auto mesh : node.Meshes) if (mesh >= parsed.Meshes.size()) return false;
            parsed.Nodes.push_back(std::move(node));
        }
        for (int i = 0; i < 3; ++i) { parsed.Minimum[i] = root["Bounds"][i].as<float>(); parsed.Maximum[i] = root["Bounds"][i + 3].as<float>(); }
        parsed.Diagnostics = root["Diagnostics"].as<std::vector<std::string>>(); parsed.Dependencies = root["Dependencies"].as<std::vector<std::string>>();
        model = std::move(parsed);
        return !model.Meshes.empty() && !model.Nodes.empty();
    }
    catch (const std::exception&) { return false; }
}

void FINALITY::ModelPipeline::PrepareMaterials(ModelData& model, const std::filesystem::path& directory, const std::filesystem::path& engineAssets)
{
    namespace fs = std::filesystem;
    fs::create_directories(directory);
    const auto project = AssetRegistry::GetProjectDirectory();
    if (project.empty() || engineAssets.empty()) return;
    for (auto& dependency : model.Dependencies)
        if (std::filesystem::path(dependency).is_absolute()) dependency = std::filesystem::path(dependency).lexically_relative(project).generic_string();
    const bool runtime = AssetRegistry::GetCacheDirectory() != project / ".Finality";
    const auto shaders = runtime ? AssetRegistry::GetCacheDirectory() / "Shaders" : project / "Assets/Imported/FinalityShaders";
    fs::create_directories(shaders);
    for (const auto* name : {"imported_model.vert","imported_model.frag"})
        if (runtime)
        {
            const auto binary = shaders / (std::string(name) + ".spv");
            if (!fs::exists(binary)) fs::copy_file(engineAssets / "shaders" / (std::string(name) + ".spv"), binary);
        }
        else if (!fs::exists(shaders / name)) fs::copy_file(engineAssets / "shaders" / name, shaders / name);
    for (size_t index = 0; index < model.Materials.size(); ++index)
    {
        auto& source = model.Materials[index];
        const auto path = directory / ("material_" + std::to_string(index) + ".fmat");
        source.Asset = path.lexically_relative(project).generic_string();
        if (!source.Texture.empty()) source.Texture = fs::path(source.Texture).lexically_relative(project).generic_string();
        model.Dependencies.push_back(source.Asset);
        if (fs::exists(path)) continue;
        MaterialDefinition material;
        material.Name = source.Name; material.Type = "Imported/Unlit";
        material.VertexShader = (shaders / "imported_model.vert").lexically_relative(project).generic_string();
        material.FragmentShader = (shaders / "imported_model.frag").lexically_relative(project).generic_string();
        material.Culling = source.DoubleSided ? CullMode::None : CullMode::Back;
        material.TextureBinding = "textureSampler"; material.TexturePath = source.Texture;
        MaterialParameterDefinition color;
        color.Name = "baseColor"; color.Type = MaterialParameterType::Vec4; color.Offset = 64;
        for (int i = 0; i < 4; ++i) color.Default[i] = source.Color[i];
        material.Properties.push_back(color);
        material.EnableBlending = source.Color.a < 1;
        if (!MaterialSerializer::Serialize(material,path.string())) throw std::runtime_error("Cannot save imported material");
    }
}
