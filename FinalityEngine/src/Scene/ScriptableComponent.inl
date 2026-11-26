#pragma once
#include "Scene/Scene.h"
#include <stdexcept>

namespace FINALITY
{
    template <typename T>
    bool ScriptableComponent::HasComponent() const
    {
        return !m_Lifetime.expired() && m_CurrentScene && m_CurrentScene->m_EntityRegistry.valid(m_EntityHandle) && m_CurrentScene->m_EntityRegistry.all_of<T>(m_EntityHandle);
    }

    template <typename T>
    T& ScriptableComponent::GetComponent()
    {
        if (!HasComponent<T>())
        {
            throw std::logic_error("Script entity is expired or its component is missing");
        }
        return m_CurrentScene->m_EntityRegistry.get<T>(m_EntityHandle);
    }
} // namespace FINALITY
