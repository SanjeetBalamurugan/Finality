#include "AssetRegistry.h"
#include "AssetManager.h"
#include <Core/Log.h>
#include <Utils/UUID.h>
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>
#include <set>
#include <stb/stb_image.h>
#include <Renderer/ShaderCompiler.h>
#include <Utils/Serializers/MaterialSerializer.h>
#include "ModelAsset.h"

namespace
{
    std::filesystem::path s_ProjectDirectory;
    std::map<uint64_t, FINALITY::AssetRecord> s_Records;

    std::string PathKey(const std::filesystem::path& path)
    {
        std::string key = path.lexically_normal().generic_string();
#ifdef _WIN32
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif
        return key;
    }

    bool InsideProject(const std::filesystem::path& path)
    {
        if (s_ProjectDirectory.empty()) return false;
        const auto relative = std::filesystem::weakly_canonical(path).lexically_relative(std::filesystem::weakly_canonical(s_ProjectDirectory));
        return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
    }
}

FINALITY::ProjectAssetType FINALITY::AssetRegistry::GetType(const std::filesystem::path& path)
{
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga" || extension == ".dds" || extension == ".ktx") return ProjectAssetType::Texture;
    if (ModelAsset::Supports(path)) return ProjectAssetType::Model;
    if (extension == ".fmat") return ProjectAssetType::Material;
    if (extension == ".vert" || extension == ".frag" || extension == ".glsl") return ProjectAssetType::Shader;
    if (extension == ".fscene") return ProjectAssetType::Scene;
    if (extension == ".cpp" || extension == ".h" || extension == ".hpp") return ProjectAssetType::Script;
    if (extension == ".fprefab") return ProjectAssetType::Prefab;
    return ProjectAssetType::Unknown;
}

void FINALITY::AssetRegistry::Open(const std::filesystem::path& projectDirectory)
{
    Close();
    s_ProjectDirectory = std::filesystem::absolute(projectDirectory).lexically_normal();
    try
    {
        const auto database = s_ProjectDirectory / ".Finality/Assets.yaml";
        if (std::filesystem::exists(database))
            for (const auto& node : YAML::LoadFile(database.string())["Assets"])
            {
                AssetRecord record;
                record.ID = node["ID"].as<uint64_t>();
                record.Path = node["Path"].as<std::string>();
                record.Source = node["Source"].as<std::string>("");
                record.Type = GetType(record.Path);
                record.Status = "Missing";
                record.Modified = node["Modified"].as<int64_t>(0);
                record.Diagnostic = node["Diagnostic"].as<std::string>("");
                if (node["Dependencies"])
                    for (const auto& dependency : node["Dependencies"]) record.Dependencies.push_back(dependency.as<uint64_t>());
                s_Records.emplace(record.ID, std::move(record));
            }
    }
    catch (const std::exception& error) { FI_CORE_WARN("Rebuilding asset database: {}", error.what()); }
    Scan();
}

void FINALITY::AssetRegistry::Close()
{
    s_Records.clear();
    s_ProjectDirectory.clear();
}

void FINALITY::AssetRegistry::Scan()
{
    namespace fs = std::filesystem;
    if (s_ProjectDirectory.empty()) return;
    for (auto& [id, record] : s_Records) record.Status = "Missing";
    std::set<uint64_t> seen;
    std::vector<uint64_t> changedModels;
    try
    {
        for (const auto& entry : fs::recursive_directory_iterator(s_ProjectDirectory / "Assets", fs::directory_options::skip_permission_denied))
        {
            if (!entry.is_regular_file() || entry.path().extension() == ".fmeta" || entry.path().extension() == ".spv" || entry.path().extension() == ".ini") continue;
            if (!InsideProject(entry.path())) continue;
            const std::string path = entry.path().lexically_relative(s_ProjectDirectory).generic_string();
            fs::path metadata = entry.path();
            metadata += ".fmeta";
            uint64_t id = 0;
            if (fs::exists(metadata))
            {
                try { id = YAML::LoadFile(metadata.string())["ID"].as<uint64_t>(); }
                catch (const std::exception& error) { FI_CORE_WARN("Invalid metadata '{}': {}", metadata.string(), error.what()); }
            }
            if (!id)
                for (const auto& [knownID, known] : s_Records)
                    if (known.Path == path) { id = knownID; break; }
            if (!id || seen.contains(id))
            {
                if (seen.contains(id)) FI_CORE_WARN("Duplicate asset identity in '{}'; assigning a new ID", path);
                do { id = static_cast<uint64_t>(UUID()); } while (!id || s_Records.contains(id));
                std::ofstream stream(metadata);
                stream << "ID: " << id << '\n';
            }
            seen.insert(id);
            if (!fs::exists(metadata))
            {
                std::ofstream stream(metadata);
                stream << "ID: " << id << '\n';
            }
            auto& record = s_Records[id];
            const int64_t modified = fs::last_write_time(entry.path()).time_since_epoch().count();
            const bool changed = record.Modified != modified;
            if (changed) AssetManager::InvalidateAssetPath(entry.path().string());
            record.ID = id;
            record.Path = path;
            record.Type = GetType(path);
            record.Modified = modified;
            record.Status = record.Type == ProjectAssetType::Unknown ? "Unsupported" : "Ready";
            if (changed) record.Diagnostic.clear();
            if (changed)
            {
                try
                {
                    if (record.Type == ProjectAssetType::Model) changedModels.push_back(record.ID);
                    if (record.Type == ProjectAssetType::Texture && entry.path().extension() != ".dds" && entry.path().extension() != ".ktx")
                    {
                        int width = 0, height = 0, channels = 0;
                        if (!stbi_info(entry.path().string().c_str(), &width, &height, &channels)) throw std::runtime_error("Texture could not be decoded");
                    }
                    if (record.Type == ProjectAssetType::Shader && ShaderCompiler::IsSourceShader(entry.path().string()))
                        if (!ShaderCompiler::EnsureCompiled(entry.path().string())) throw std::runtime_error("Shader compilation failed. See Console.");
                    if (record.Type == ProjectAssetType::Material)
                    {
                        MaterialDefinition definition;
                        if (!MaterialSerializer::Deserialize(entry.path().string(), definition)) throw std::runtime_error("Invalid material definition");
                    }
                }
                catch (const std::exception& error) { record.Diagnostic = error.what(); FI_CORE_ERROR("Asset '{}': {}", record.Path, record.Diagnostic); }
            }
            if (!record.Diagnostic.empty()) record.Status = "Error";
        }
        for (auto id : changedModels)
        {
            auto& record = s_Records.at(id);
            try
            {
                ModelAsset::Load("asset://" + std::to_string(id));
            }
            catch (const std::exception& error) { record.Status = "Error"; record.Diagnostic = error.what(); FI_CORE_ERROR("Model '{}': {}",record.Path,error.what()); }
        }
        if (!changedModels.empty()) Scan();
        for (auto id : changedModels)
        {
            auto& record = s_Records.at(id);
            record.Dependencies.clear();
            if (record.Status != "Ready") continue;
            const auto model = ModelAsset::Load("asset://" + std::to_string(id));
            for (const auto& dependency : model->Dependencies)
                if (const auto* asset = FindPath(dependency)) record.Dependencies.push_back(asset->ID);
        }
        Save();
    }
    catch (const std::exception& error) { FI_CORE_ERROR("Asset scan failed: {}", error.what()); }
}

void FINALITY::AssetRegistry::Reimport(uint64_t id)
{
    const auto found = s_Records.find(id);
    if (found == s_Records.end()) return;
    auto& record = found->second;
    if (record.Type == ProjectAssetType::Model && record.Status != "Missing")
        try { ModelAsset::Load("asset://" + std::to_string(id),true); }
        catch (const std::exception& error)
        {
            record.Status = "Error"; record.Diagnostic = error.what();
            FI_CORE_ERROR("Model '{}': {}",record.Path,error.what());
            Save();
            return;
        }
    record.Modified = 0;
    Scan();
}

bool FINALITY::AssetRegistry::Save()
{
    if (s_ProjectDirectory.empty()) return false;
    std::filesystem::create_directories(s_ProjectDirectory / ".Finality");
    YAML::Emitter out;
    out << YAML::BeginMap << YAML::Key << "Version" << YAML::Value << 1;
    out << YAML::Key << "Assets" << YAML::Value << YAML::BeginSeq;
    for (const auto& [id, record] : s_Records)
    {
        out << YAML::BeginMap << YAML::Key << "ID" << YAML::Value << id;
        out << YAML::Key << "Path" << YAML::Value << record.Path;
        out << YAML::Key << "Source" << YAML::Value << record.Source;
        out << YAML::Key << "Modified" << YAML::Value << record.Modified;
        out << YAML::Key << "Status" << YAML::Value << record.Status;
        out << YAML::Key << "Diagnostic" << YAML::Value << record.Diagnostic;
        out << YAML::Key << "Dependencies" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (auto dependency : record.Dependencies) out << dependency;
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq << YAML::EndMap;
    std::ofstream stream(s_ProjectDirectory / ".Finality/Assets.yaml");
    stream << out.c_str();
    return stream.good();
}

bool FINALITY::AssetRegistry::Import(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    namespace fs = std::filesystem;
    try
    {
        const auto folder = s_ProjectDirectory / destination;
        const auto relative = folder.lexically_normal().lexically_relative(s_ProjectDirectory / "Assets");
        if (relative.empty() || relative.is_absolute() || *relative.begin() == ".." || !InsideProject(folder))
            throw std::runtime_error("Import destination must be inside Assets");
        fs::create_directories(folder);
        const auto target = folder / source.filename();
        if (fs::exists(target) && !fs::equivalent(source, target))
            throw std::runtime_error("An asset with that name already exists");
        if (!fs::exists(target))
        {
            if (fs::is_regular_file(source) && ModelAsset::Supports(source))
            {
                if (!ModelAsset::Import(source,target)) return false;
            }
            else fs::copy(source, target, fs::copy_options::recursive);
        }
        Scan();
        for (auto& [id, record] : s_Records)
            if ((s_ProjectDirectory / record.Path) == target) record.Source = source.generic_string();
        Save();
        return true;
    }
    catch (const std::exception& error) { FI_CORE_ERROR("Import failed: {}", error.what()); return false; }
}

bool FINALITY::AssetRegistry::Move(uint64_t id, const std::filesystem::path& destination)
{
    const auto* record = Find(id);
    if (!record) return false;
    try
    {
        const auto target = s_ProjectDirectory / destination;
        const auto relative = target.lexically_normal().lexically_relative(s_ProjectDirectory / "Assets");
        if (relative.empty() || relative.is_absolute() || *relative.begin() == ".." || !InsideProject(target) || std::filesystem::exists(target)) return false;
        const auto source = s_ProjectDirectory / record->Path;
        std::filesystem::rename(source, target);
        if (std::filesystem::exists(source.string() + ".fmeta"))
            std::filesystem::rename(source.string() + ".fmeta", target.string() + ".fmeta");
        Scan();
        return true;
    }
    catch (const std::exception& error) { FI_CORE_ERROR("Asset move failed: {}", error.what()); return false; }
}

const std::map<uint64_t, FINALITY::AssetRecord>& FINALITY::AssetRegistry::GetRecords() { return s_Records; }
const FINALITY::AssetRecord* FINALITY::AssetRegistry::Find(uint64_t id)
{
    const auto found = s_Records.find(id);
    return found == s_Records.end() ? nullptr : &found->second;
}
const FINALITY::AssetRecord* FINALITY::AssetRegistry::FindPath(const std::filesystem::path& path)
{
    const auto absolute = path.is_absolute() ? path.lexically_normal() : (s_ProjectDirectory / path).lexically_normal();
    for (const auto& [id, record] : s_Records)
        if (PathKey(s_ProjectDirectory / record.Path) == PathKey(absolute)) return &record;
    return nullptr;
}
std::string FINALITY::AssetRegistry::Resolve(const std::string& reference)
{
    if (reference.starts_with("asset://"))
    {
        try { if (const auto* record = Find(std::stoull(reference.substr(8)))) return (s_ProjectDirectory / record->Path).string(); }
        catch (const std::exception&) {}
        return {};
    }
    if (reference.empty() || reference.starts_with("builtin://") || s_ProjectDirectory.empty()) return reference;
    if (reference.starts_with("assets/")) return (s_ProjectDirectory / "Assets" / reference.substr(7)).string();
    const std::filesystem::path path(reference);
    return path.is_absolute() ? reference : (s_ProjectDirectory / path).string();
}
std::string FINALITY::AssetRegistry::Reference(const std::filesystem::path& path)
{
    if (const auto* record = FindPath(path)) return "asset://" + std::to_string(record->ID);
    return path.generic_string();
}
const std::filesystem::path& FINALITY::AssetRegistry::GetProjectDirectory() { return s_ProjectDirectory; }
