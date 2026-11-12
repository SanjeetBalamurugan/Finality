#pragma once

#include <Core/Core.h>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <cstdint>

namespace FINALITY
{
    enum class ProjectAssetType { Unknown, Texture, Model, Material, Shader, Scene, Script, Prefab };

    struct AssetRecord
    {
        uint64_t ID = 0;
        std::string Path;
        std::string Source;
        ProjectAssetType Type = ProjectAssetType::Unknown;
        std::string Status = "Ready";
        std::string Diagnostic;
        int64_t Modified = 0;
        std::vector<uint64_t> Dependencies;
    };

    class FAPI AssetRegistry
    {
    public:
        static void Open(const std::filesystem::path& projectDirectory);
        static void Close();
        static void Scan();
        static void Reimport(uint64_t id);
        static bool Import(const std::filesystem::path& source, const std::filesystem::path& destination = "Assets");
        static bool Move(uint64_t id, const std::filesystem::path& destination);
        static bool Save();
        static const std::map<uint64_t, AssetRecord>& GetRecords();
        static const AssetRecord* Find(uint64_t id);
        static const AssetRecord* FindPath(const std::filesystem::path& path);
        static std::string Resolve(const std::string& reference);
        static std::string Reference(const std::filesystem::path& path);
        static const std::filesystem::path& GetProjectDirectory();
        static ProjectAssetType GetType(const std::filesystem::path& path);
    };
}
