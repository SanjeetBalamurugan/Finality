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

    struct ModelMesh
    {
        std::string Name;
        std::string Key;
        uint32_t Material = 0;
        MeshGeometry Geometry;
    };

    struct ModelMaterial
    {
        std::string Name;
        glm::vec4 Color{1};
        std::string Texture;
        std::string Asset;
        bool DoubleSided = false;
        std::map<std::string,std::string> Properties;
    };

    struct ModelNode
    {
        std::string Name;
        int Parent = -1;
        glm::mat4 Transform{1};
        std::vector<uint32_t> Meshes;
    };

    struct ModelData
    {
        std::vector<ModelMesh> Meshes;
        std::vector<ModelMaterial> Materials;
        std::vector<ModelNode> Nodes;
        std::vector<std::string> Dependencies;
        std::vector<std::string> Diagnostics;
        glm::vec3 Minimum{0}, Maximum{0};
    };

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
    };
}
