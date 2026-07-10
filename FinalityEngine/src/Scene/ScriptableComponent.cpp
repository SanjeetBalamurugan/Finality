#include "ScriptableComponent.h"
#include "Entity.h"

namespace FINALITY
{
    Entity ScriptableComponent::GetEntity()
    {
        return Entity{ m_EntityHandle, m_CurrentScene };
    }

    const Entity ScriptableComponent::GetEntity() const
    {
        return Entity{ m_EntityHandle, m_CurrentScene };
    }
}
