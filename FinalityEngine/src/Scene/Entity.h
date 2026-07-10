#pragma once

#include <Core/Core.h>
#include "Scene.h"

#include <entt.hpp>

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

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_CurrentScene = nullptr;

        friend class Scene;
        friend class ScriptableComponent;
    };
}

#include "Entity.inl"