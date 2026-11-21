#include "ModelPipeline.h"
#include "ModelImportSettings.h"
#include "AssetRegistry.h"
#include "AssetManager.h"
#include "MaterialAsset.h"
#include <Renderer/Renderer.h>
#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <ImGUI/ImGuizmoBridge.h>
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <unordered_map>
#include <Core/Log.h>
#include <algorithm>
#include <cmath>

namespace
{
    std::filesystem::path s_EngineAssets;
    FINALITY::ModelImportOptions s_Options;
    std::unordered_map<std::string,std::pair<std::string,std::shared_ptr<FINALITY::ModelData>>> s_Models;
    std::unordered_map<std::string,std::weak_ptr<FINALITY::Mesh>> s_Meshes;

    std::string Source(const std::string& reference) { return FINALITY::AssetRegistry::Resolve(reference.substr(0,reference.find('#'))); }
    std::string Signature(const std::filesystem::path& path)
    {
        const auto options = FINALITY::ModelImportSettings::Read(path, s_Options);
        return "meters-v4:" + std::to_string(std::filesystem::last_write_time(path).time_since_epoch().count()) + ":" + std::to_string(std::filesystem::file_size(path)) + ":" + std::to_string(options.Scale) + ":" + std::to_string(options.GenerateNormals) + std::to_string(options.FlipUVs) + std::to_string(options.ImportMaterials) + std::to_string(options.ConvertUnits) + (std::filesystem::exists(path.string()+".fmeta") ? std::to_string(std::filesystem::last_write_time(path.string()+".fmeta").time_since_epoch().count()) : "");
    }
    uint64_t Identity(const std::filesystem::path& path)
    {
        if (const auto* record = FINALITY::AssetRegistry::FindPath(path)) return record->ID;
        const auto metadata = path.string() + ".fmeta";
        if (std::filesystem::exists(metadata)) return YAML::LoadFile(metadata)["ID"].as<uint64_t>();
        return 0;
    }
    std::filesystem::path CacheDirectory(const std::filesystem::path& path)
    {
        const auto id = Identity(path);
        return !id || FINALITY::AssetRegistry::GetProjectDirectory().empty() ? std::filesystem::path() : FINALITY::AssetRegistry::GetCacheDirectory() / "Models" / std::to_string(id);
    }
    std::filesystem::path ResourceDirectory(const std::filesystem::path& path)
    {
        const auto id = Identity(path);
        return !id || FINALITY::AssetRegistry::GetProjectDirectory().empty() ? std::filesystem::path() : FINALITY::AssetRegistry::GetProjectDirectory() / "Assets/Imported" / std::to_string(id);
    }
    void ApplyMaterialRemaps(FINALITY::ModelData& model, const std::filesystem::path& path)
    {
        if (!std::filesystem::is_regular_file(path.string()+".fmeta")) return;
        const auto remaps = YAML::LoadFile(path.string()+".fmeta")["ModelImporter"]["MaterialRemaps"];
        if (!remaps || !remaps.IsSequence()) return;
        for (size_t i = 0; i < std::min(remaps.size(),model.Materials.size()); ++i)
        {
            const auto asset = remaps[i].as<std::string>();
            if (std::filesystem::is_regular_file(FINALITY::AssetRegistry::Resolve(asset))) model.Materials[i].Asset = asset;
        }
    }
    std::vector<glm::mat4> WorldTransforms(const FINALITY::ModelData& model)
    {
        std::vector<glm::mat4> result;
        for (const auto& node : model.Nodes) result.push_back(node.Parent < 0 ? node.Transform : result.at(node.Parent) * node.Transform);
        return result;
    }
}

void FINALITY::ModelAsset::Configure(const std::filesystem::path& engineAssets, const ModelImportOptions& options)
{
    if (!(s_Options == options) || s_EngineAssets != engineAssets) Clear();
    s_EngineAssets = engineAssets; s_Options = options;
}
void FINALITY::ModelAsset::Clear() { s_Models.clear(); s_Meshes.clear(); }
bool FINALITY::ModelAsset::Supports(const std::filesystem::path& path)
{
    auto extension = path.extension().string();
    std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".obj" || extension == ".gltf" || extension == ".glb" || extension == ".fbx";
}

std::shared_ptr<FINALITY::ModelData> FINALITY::ModelAsset::Load(const std::string& reference, bool force)
{
    const std::filesystem::path path(Source(reference));
    const auto signature = Signature(path);
    const auto key = path.lexically_normal().string();
    if (force) s_Meshes.clear();
    if (!force)
        if (const auto found = s_Models.find(key); found != s_Models.end() && found->second.first == signature) return found->second.second;
    const auto cache = CacheDirectory(path);
    auto model = std::make_shared<ModelData>();
    if (force || cache.empty() || !ModelPipeline::ReadCache(*model,cache,signature))
    {
        const auto resources = ResourceDirectory(path);
        *model = ModelPipeline::Read(path,GetImportOptions(reference),resources.empty() ? resources : resources / "Textures");
        if (!resources.empty())
        {
            ModelPipeline::PrepareMaterials(*model,resources,s_EngineAssets);
            ApplyMaterialRemaps(*model,path);
            ModelPipeline::Save(*model,cache,signature);
        }
    }
    for (const auto& diagnostic : model->Diagnostics) FI_CORE_WARN("Model '{}': {}", path.filename().string(),diagnostic);
    s_Models[key] = {signature,model};
    return model;
}

bool FINALITY::ModelAsset::Import(const std::filesystem::path& source, const std::filesystem::path& target)
{
    try
    {
        namespace fs = std::filesystem;
        if (!fs::exists(target) || !fs::equivalent(source,target)) fs::copy_file(source,target,fs::copy_options::overwrite_existing);
        const auto metadata = target.string() + ".fmeta";
        if (!fs::exists(metadata)) { std::ofstream stream(metadata); stream << "ID: " << static_cast<uint64_t>(UUID()) << '\n'; }
        const auto resources = ResourceDirectory(target);
        MakePortable(target,source.parent_path());
        auto extension = target.extension().string();
        std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const auto& readable = extension == ".gltf" || extension == ".glb" ? target : source;
        auto model = std::make_shared<ModelData>(ModelPipeline::Read(readable,ModelImportSettings::Read(target,s_Options),resources / "Textures"));
        ModelPipeline::PrepareMaterials(*model,resources,s_EngineAssets);
        const auto signature = Signature(target);
        ApplyMaterialRemaps(*model,target);
        ModelPipeline::Save(*model,CacheDirectory(target),signature);
        s_Models[target.lexically_normal().string()] = {signature,model};
        return true;
    }
    catch (const std::exception& error) { FI_CORE_ERROR("Model import failed: {}",error.what()); return false; }
}

std::shared_ptr<FINALITY::Mesh> FINALITY::ModelAsset::LoadMesh(const std::string& reference)
{
    auto model = Load(reference);
    const auto key = reference + ":" + Signature(Source(reference));
    if (auto cached = s_Meshes[key].lock()) return cached;
    MeshGeometry geometry;
    const auto fragment = reference.find("#mesh=");
    if (fragment != std::string::npos)
    {
        const auto name = reference.substr(fragment + 6);
        const auto part = std::find_if(model->Meshes.begin(),model->Meshes.end(),[&](const auto& mesh) { return mesh.Key == name; });
        if (part == model->Meshes.end()) throw std::runtime_error("Model submesh no longer exists: " + name);
        geometry = part->Geometry;
    }
    else
    {
        const auto world = WorldTransforms(*model);
        for (size_t node = 0; node < model->Nodes.size(); ++node)
            for (auto part : model->Nodes[node].Meshes)
            {
                const auto& mesh = model->Meshes.at(part).Geometry;
                const auto offset = static_cast<uint32_t>(geometry.Vertices.size());
                const glm::mat3 linear(world[node]);
                const auto normalMatrix = std::abs(glm::determinant(linear)) > 1.0e-8f ? glm::transpose(glm::inverse(linear)) : glm::mat3(1);
                for (auto vertex : mesh.Vertices)
                {
                    vertex.Position = glm::vec3(world[node] * glm::vec4(vertex.Position,1));
                    const auto normal = normalMatrix * vertex.Normal;
                    const auto tangent = linear * glm::vec3(vertex.Tangent);
                    if (glm::dot(normal,normal) > 0) vertex.Normal = glm::normalize(normal);
                    if (glm::dot(tangent,tangent) > 0) vertex.Tangent = glm::vec4(glm::normalize(tangent),vertex.Tangent.w);
                    geometry.Vertices.push_back(vertex);
                }
                for (auto index : mesh.Indices) geometry.Indices.push_back(offset + index);
            }
    }
    if (geometry.Vertices.empty() || geometry.Indices.empty()) return nullptr;
    auto mesh = Renderer::GetDevice()->CreateMesh(geometry.Vertices,geometry.Indices);
    s_Meshes[key] = mesh;
    return mesh;
}

FINALITY::Entity FINALITY::ModelAsset::Instantiate(Scene& scene, const std::string& reference)
{
    const auto model = Load(reference);
    const auto base = AssetRegistry::Reference(Source(reference));
    auto root = scene.CreateEntity(std::filesystem::path(Source(reference)).stem().string());
    std::vector<Entity> entities;
    for (const auto& node : model->Nodes)
    {
        auto entity = scene.CreateEntity(node.Name);
        entity.SetParent(node.Parent < 0 ? root : entities.at(node.Parent));
        auto& transform = entity.GetComponent<TransformComponent>();
        ImGuizmoBridge::SetLocalTransform(transform,node.Transform);
        entities.push_back(entity);
        for (auto index : node.Meshes)
        {
            const auto& mesh = model->Meshes.at(index);
            auto part = scene.CreateEntity(mesh.Name); part.SetParent(entity);
            const auto meshReference = base + "#mesh=" + mesh.Key;
            part.AddComponent<MeshComponent>(LoadMesh(meshReference),meshReference);
            std::string materialPath = mesh.Material < model->Materials.size() && GetImportOptions(reference).ImportMaterials ? model->Materials[mesh.Material].Asset : "Assets/materials/Default.fmat";
            if (materialPath.empty()) materialPath = "Assets/materials/Default.fmat";
            const auto materialReference = AssetRegistry::Reference(materialPath);
            if (auto material = AssetManager::CreateAsset<MaterialAsset>(materialReference,Renderer::GetDevice()))
                if (auto instance = material->CreateInstance()) part.AddComponent<MaterialComponent>(instance,materialReference);
        }
    }
    return root;
}

std::vector<FINALITY::RenderPacket> FINALITY::ModelAsset::BuildPreview(const std::string& reference)
{
    const auto model = Load(reference);
    const auto world = WorldTransforms(*model);
    const auto base = AssetRegistry::Reference(Source(reference));
    std::vector<RenderPacket> result;
    for (size_t node = 0; node < model->Nodes.size(); ++node)
        for (auto index : model->Nodes[node].Meshes)
        {
            const auto& mesh = model->Meshes.at(index);
            const auto materialPath = mesh.Material < model->Materials.size() && GetImportOptions(reference).ImportMaterials ? model->Materials[mesh.Material].Asset : std::string();
            auto asset = AssetManager::CreateAsset<MaterialAsset>(materialPath.empty() ? "Assets/materials/Default.fmat" : materialPath,Renderer::GetDevice());
            if (!asset) continue;
            auto material = asset->CreateInstance();
            if (!material) continue;
            RenderPacket packet;
            packet.MeshData = LoadMesh(base + "#mesh=" + mesh.Key); packet.Transform = world[node]; packet.PipelineInstance = material->GetPipeline();
            packet.Textures = material->GetTextures(); packet.CustomPushData = material->GetRawDataBuffer(); packet.MaterialInstance = material;
            result.push_back(std::move(packet));
        }
    return result;
}

FINALITY::ModelImportOptions FINALITY::ModelAsset::GetImportOptions(const std::string& reference)
{
    return ModelImportSettings::Read(Source(reference), s_Options);
}
bool FINALITY::ModelAsset::SaveImportOptions(const std::string& reference, const ModelImportOptions& options, std::string& error)
{
    if (!ModelImportSettings::Save(Source(reference), options, error)) return false;
    Clear(); return true;
}

bool FINALITY::ModelAsset::ExtractMaterials(const std::string& reference, const std::filesystem::path& directory, std::string& error)
{
    try
    {
        const auto root = std::filesystem::weakly_canonical(AssetRegistry::GetProjectDirectory()/"Assets");
        const auto target = std::filesystem::weakly_canonical(directory);
        const auto relative = target.lexically_relative(root);
        if (relative.empty() || relative.is_absolute() || *relative.begin() == "..") throw std::runtime_error("Choose a folder inside this project's Assets directory");
        auto model = Load(reference);
        std::vector<std::string> extracted;
        for (size_t i = 0; i < model->Materials.size(); ++i)
        {
            const auto source = std::filesystem::path(AssetRegistry::Resolve(model->Materials[i].Asset));
            if (!std::filesystem::is_regular_file(source)) throw std::runtime_error("An imported material is missing");
            auto output = target/(std::filesystem::path(Source(reference)).stem().string()+"_material_"+std::to_string(i)+".fmat");
            for (int suffix = 1; std::filesystem::exists(output); ++suffix)
                output = target/(std::filesystem::path(Source(reference)).stem().string()+"_material_"+std::to_string(i)+"_"+std::to_string(suffix)+".fmat");
            std::filesystem::copy_file(source,output);
            extracted.push_back(output.lexically_relative(AssetRegistry::GetProjectDirectory()).generic_string());
        }
        AssetRegistry::Scan();
        for (auto& asset : extracted) asset = AssetRegistry::Reference(asset);
        auto path = std::filesystem::path(Source(reference)); path += ".fmeta";
        auto metadata = YAML::LoadFile(path.string());
        metadata["ModelImporter"]["MaterialRemaps"] = extracted;
        auto temporary = path; temporary += ".tmp";
        { std::ofstream stream(temporary); stream << metadata; if (!stream) throw std::runtime_error("Cannot save material mappings"); }
        std::filesystem::copy_file(temporary,path,std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(temporary);
        Clear(); AssetRegistry::Scan();
        error = "Materials extracted and mapped for new model instances";
        return true;
    }
    catch (const std::exception& failure) { error = failure.what(); return false; }
}
