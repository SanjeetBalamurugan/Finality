#pragma once

#include "ScriptableComponent.h"

namespace FINALITY
{
    template<typename T, typename... Args>
    T& Entity::AddScript(Args&&... args)
    {
        static_assert(std::is_base_of_v<ScriptableComponent, T>,
            "T must inherit from ScriptableComponent");

        if (!HasComponent<ScriptStorage>())
            m_CurrentScene->m_EntityRegistry.emplace<ScriptStorage>(m_EntityHandle);

        auto& storage = GetComponent<ScriptStorage>();

        auto script = std::make_unique<T>(std::forward<Args>(args)...);

        script->m_EntityHandle = m_EntityHandle;
        script->m_CurrentScene = m_CurrentScene;

        T* ptr = script.get();

        storage.Scripts.emplace_back(std::move(script), false);

        ptr->Awake();

        return *ptr;
    }

    template<typename T, typename... Args>
    T& Entity::AddComponent(Args&&... args)
    {
        assert(!HasComponent<T>() && "Component already exists!");

        return m_CurrentScene->m_EntityRegistry.emplace<T>(
            m_EntityHandle,
            std::forward<Args>(args)...);
    }

    template<typename T>
    T& Entity::GetComponent()
    {
        assert(HasComponent<T>() && "Component doesn't exist!");

        return m_CurrentScene->m_EntityRegistry.get<T>(m_EntityHandle);
    }

    template<typename T>
    bool Entity::HasComponent() const
    {
        return m_CurrentScene->m_EntityRegistry.all_of<T>(m_EntityHandle);
    }

    template<typename T>
    void Entity::RemoveComponent()
    {
        assert(HasComponent<T>() && "Component doesn't exist!");

        m_CurrentScene->m_EntityRegistry.remove<T>(m_EntityHandle);
    }
}
