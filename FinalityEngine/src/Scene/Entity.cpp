#include "Entity.h"

namespace FINALITY
{
    Entity::Entity(entt::entity handle, Scene* scene)
        : m_EntityHandle(handle), m_CurrentScene(scene)
    {
    }
}