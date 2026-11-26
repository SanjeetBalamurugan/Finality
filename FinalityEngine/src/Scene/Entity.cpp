#include "Entity.h"

namespace FINALITY
{
    Entity::Entity() = default;
    Entity::Entity(const Entity&) = default;
    Entity::Entity(Entity&&) noexcept = default;
    Entity& Entity::operator=(const Entity&) = default;
    Entity& Entity::operator=(Entity&&) noexcept = default;
    Entity::~Entity() = default;

    Entity::Entity(entt::entity handle, Scene* scene)
        : m_EntityHandle(handle), m_CurrentScene(scene), m_Lifetime(scene ? scene->GetLifetime() : std::weak_ptr<void>{})
    {
    }
} // namespace FINALITY
