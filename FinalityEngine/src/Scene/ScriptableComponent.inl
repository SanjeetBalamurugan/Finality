#pragma once

#include "Entity.h"

namespace FINALITY
{
    template<typename T>
    T& ScriptableComponent::GetComponent()
    {
        return m_Entity->GetComponent<T>();
    }


    template<typename T>
    bool ScriptableComponent::HasComponent() const
    {
        return m_Entity->HasComponent<T>();
    }
}