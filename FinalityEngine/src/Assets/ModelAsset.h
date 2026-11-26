/**
 * @file
 * @ingroup assets
 * @brief Loads imported model data and instantiates preserved model hierarchies.
 *
 * ModelData retains nodes, submeshes, material slots, dependencies, diagnostics and bounds. ModelAsset applies import settings and supplies both scene instantiation and preview render packets.
 *
 * @par Usage
 * Configure engine assets first. Load a model reference, inspect diagnostics, then Instantiate into a scene. SaveImportOptions persists per-model adjustments; ExtractMaterials writes editable material assets.
 */
#pragma once

#include <Core/Core.h>
#include "MeshImporter.h"
#include "ModelImportOptions.h"
#include <filesystem>
#include <map>

namespace FINALITY
{
    class Scene;
    class Entity;
    struct RenderPacket;

    /** @brief One imported submesh, with geometry and an index into ModelData::Materials. */
    struct ModelMesh
    {
        std::string Name;
        std::string Key;
        uint32_t Material = 0;
        MeshGeometry Geometry;
    };

    /** @brief Imported material appearance, texture reference and extracted asset mapping. */
    struct ModelMaterial
    {
        std::string Name;
        glm::vec4 Color{ 1 };
        std::string Texture;
        std::string Asset;
        bool DoubleSided = false;
        std::map<std::string, std::string> Properties;
    };

    /** @brief One hierarchy node with a parent index, local transform and attached mesh indices. A parent of -1 identifies a root. */
    struct ModelNode
    {
        std::string Name;
        int Parent = -1;
        glm::mat4 Transform{ 1 };
        std::vector<uint32_t> Meshes;
    };

    /** @brief Imported nodes, submeshes, materials, dependencies, diagnostics and model-space bounds. */
    struct ModelData
    {
        std::vector<ModelMesh> Meshes;
        std::vector<ModelMaterial> Materials;
        std::vector<ModelNode> Nodes;
        std::vector<std::string> Dependencies;
        std::vector<std::string> Diagnostics;
        glm::vec3 Minimum{ 0 }, Maximum{ 0 };
    };

    /**
     * @ingroup assets
     * @brief Loads imported model data and instantiates preserved model hierarchies.
     *
     * ModelData retains nodes, submeshes, material slots, dependencies, diagnostics and bounds. ModelAsset applies import settings and supplies both scene instantiation and preview render packets.
     *
     * @par Usage
     * Configure engine assets first. Load a model reference, inspect diagnostics, then Instantiate into a scene. SaveImportOptions persists per-model adjustments; ExtractMaterials writes editable material assets.
     */
    class FAPI ModelAsset
    {
    public:
        static void Configure(const std::filesystem::path& engineAssets, const ModelImportOptions& options = {});
        static std::shared_ptr<ModelData> Load(const std::string& reference, bool force = false);
        static bool Import(const std::filesystem::path& source, const std::filesystem::path& target);
        static void MakePortable(const std::filesystem::path& path, const std::filesystem::path& sourceDirectory = {});
        static std::shared_ptr<Mesh> LoadMesh(const std::string& reference);
        static Entity Instantiate(Scene& scene, const std::string& reference);
        static std::vector<RenderPacket> BuildPreview(const std::string& reference);
        static void Clear();
        static bool Supports(const std::filesystem::path& path);
        static ModelImportOptions GetImportOptions(const std::string& reference);
        static bool SaveImportOptions(const std::string& reference, const ModelImportOptions& options, std::string& error);
        static bool ExtractMaterials(const std::string& reference, const std::filesystem::path& directory, std::string& error);
    };
} // namespace FINALITY
