/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Prefab.h.
 */
#pragma once
#include <Finality.h>
#include <set>

namespace FINALITY
{
    inline bool SavePrefab(Scene& source, entt::entity root, const std::filesystem::path& path)
    {
        if (!source.GetRegistry().valid(root)) return false;
        EmptyScene copy;
        if (!copy.InstantiateHierarchy(source, root).IsValid()) return false;
        std::filesystem::create_directories(path.parent_path());
        SceneSerializer(copy, path.stem().string()).Serialize(path.string());
        AssetRegistry::Scan();
        return std::filesystem::is_regular_file(path);
    }

    inline Entity InstantiatePrefab(Scene& destination, const std::filesystem::path& path)
    {
        EmptyScene source;
        if (!SceneSerializer(source).DeSerialize(path.string())) return {};
        for (auto entity : source.GetRegistry().view<RelationshipComponent>())
            if (source.GetRegistry().get<RelationshipComponent>(entity).Parent == entt::null)
                return destination.InstantiateHierarchy(source, entity);
        return {};
    }
}
