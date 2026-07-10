#include "ScriptableComponent.h"
#include "Entity.h"

namespace FINALITY
{
    Entity& ScriptableComponent::GetEntity()
    {
        return *m_Entity;
    }


    const Entity& ScriptableComponent::GetEntity() const
    {
        return *m_Entity;
    }
}