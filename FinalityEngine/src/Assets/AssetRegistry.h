/**
 * @file
 * @ingroup assets
 * @brief Tracks project assets, metadata and reference resolution.
 *
 * The registry maps stable asset IDs to project-relative paths and records dependencies, status and diagnostics. Runtime cache location can be redirected so a packaged game does not write imported data into its installation.
 *
 * @par Usage
 * Open the project before scanning or resolving its references. Use Move for asset moves so metadata stays associated with the source asset. Resolve converts stored references to usable paths.
 */
#pragma once

#include <Core/Core.h>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <cstdint>

namespace FINALITY
{
    /** @brief Asset categories used by registry discovery and editor inspection. */
    enum class ProjectAssetType
    {
        Unknown,
        Texture,
        Model,
        Material,
        Shader,
        Scene,
        Script,
        Prefab
    };

    /** @brief Registry entry associating a stable asset ID with a project-relative path, dependencies and import status. */
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

    /**
     * @ingroup assets
     * @brief Tracks project assets, metadata and reference resolution.
     *
     * The registry maps stable asset IDs to project-relative paths and records dependencies, status and diagnostics. Runtime cache location can be redirected so a packaged game does not write imported data into its installation.
     *
     * @par Usage
     * Open the project before scanning or resolving its references. Use Move for asset moves so metadata stays associated with the source asset. Resolve converts stored references to usable paths.
     */
    class FAPI AssetRegistry
    {
    public:
        static void Open(const std::filesystem::path& projectDirectory);
        /**
         * @brief Overrides where runtime import/database caches are written.
         * @param directory Writable cache directory for this runtime instance or project.
         * Packaged players use local application data so imported data does not modify the
         * installed game assets. Configure this before loading runtime assets.
         */
        static void SetRuntimeCacheDirectory(const std::filesystem::path& directory);
        static std::filesystem::path GetCacheDirectory();
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
} // namespace FINALITY
