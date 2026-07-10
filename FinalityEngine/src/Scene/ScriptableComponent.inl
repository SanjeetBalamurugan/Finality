#pragma once
#include "Scene/Scene.h"

namespace FINALITY
{
    template<typename T>
    bool ScriptableComponent::HasComponent() const
    {
        return m_CurrentScene->m_EntityRegistry.all_of<T>(m_EntityHandle);
    }

    template<typename T>
    T& ScriptableComponent::GetComponent()
    {
        return m_CurrentScene->m_EntityRegistry.get<T>(m_EntityHandle);
    }
}
