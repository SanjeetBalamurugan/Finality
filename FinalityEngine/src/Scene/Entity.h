#pragma once

#include <Core/Core.h>
#include "Scene.h"

#include <entt.hpp>
#include <Utils/UUID.h>

namespace FINALITY
{
    class ScriptableComponent;
    struct ScriptStorage;

    class FAPI Entity
    {
    private:
        Entity(entt::entity handle, Scene* scene);

    public:
        Entity() = default;

        template<typename T, typename... Args>
        T& AddScript(Args&&... args);

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T>
        bool HasComponent() const;

        template<typename T>
        void RemoveComponent();

        entt::entity GetHandle() const { return m_EntityHandle; }
        bool IsValid() const { return m_EntityHandle != entt::null; }

        void SetParent(Entity parent);

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_CurrentScene = nullptr;

        friend class Scene;
        friend class SceneSerializer;
        friend class ScriptableComponent;
    };

    // For entity to have children entity (need to frame this sentence correctly)
    struct RelationshipComponent
    {
        entt::entity Parent = entt::null;
        entt::entity FirstChild = entt::null;
        entt::entity NextSibling = entt::null;
        entt::entity PrevSibling = entt::null;
    };
}

#include "Entity.inl"