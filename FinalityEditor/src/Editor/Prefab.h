#pragma once
#include <Finality.h>
#include <set>

namespace FINALITY
{
    inline bool SavePrefab(Scene& source, entt::entity root, const std::filesystem::path& path)
    {
        if (!source.GetRegistry().valid(root)) return false;
        const auto identity = source.GetRegistry().get<TagComponent>(root).uuid;
        EmptyScene copy;
        if (!SceneSerializer(copy).DeSerializeFromString(SceneSerializer(source).SerializeToString())) return false;
        entt::entity selected = entt::null;
        for (auto entity : copy.GetRegistry().view<TagComponent>())
            if (copy.GetRegistry().get<TagComponent>(entity).uuid == identity) selected = entity;
        if (selected == entt::null) return false;
        copy.GetEntity(selected).SetParent({});
        std::vector<entt::entity> remove;
        for (auto entity : copy.GetRegistry().view<RelationshipComponent>())
            if (entity != selected && copy.GetRegistry().get<RelationshipComponent>(entity).Parent == entt::null) remove.push_back(entity);
        for (auto entity : remove) copy.DestroyEntity(entity);
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
