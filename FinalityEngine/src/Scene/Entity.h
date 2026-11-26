/**
 * @file
 * @ingroup scene
 * @brief Provides a scene-bound handle for component and hierarchy operations.
 *
 * Entity wraps an EnTT handle and a non-owning Scene pointer. A weak lifetime token prevents IsValid from accepting handles whose scene has expired. Copying the wrapper does not clone the underlying entity.
 *
 * @par Usage
 * Check IsValid before using a stored handle. Use Scene::DuplicateEntity to copy an entity hierarchy; use SetParent to change hierarchy relationships.
 */
#pragma once

#include <Core/Core.h>
#include "Scene.h"

#include <entt.hpp>
#include <Utils/UUID.h>

namespace FINALITY
{
    class ScriptableComponent;
    struct ScriptStorage;

    /**
     * @ingroup scene
     * @brief Provides a scene-bound handle for component and hierarchy operations.
     *
     * Entity wraps an EnTT handle and a non-owning Scene pointer. A weak lifetime token prevents IsValid from accepting handles whose scene has expired. Copying the wrapper does not clone the underlying entity.
     *
     * @par Usage
     * Check IsValid before using a stored handle. Use Scene::DuplicateEntity to copy an entity hierarchy; use SetParent to change hierarchy relationships.
     */
    class FAPI Entity
    {
    private:
        Entity(entt::entity handle, Scene* scene);

    public:
        Entity();
        Entity(const Entity&);
        Entity(Entity&&) noexcept;
        Entity& operator=(const Entity&);
        Entity& operator=(Entity&&) noexcept;
        ~Entity();

        template <typename T, typename... Args>
        T& AddScript(Args&&... args);

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template <typename T>
        T& GetComponent();

        template <typename T>
        bool HasComponent() const;

        template <typename T>
        void RemoveComponent();

        entt::entity GetHandle() const
        {
            return m_EntityHandle;
        }

        /**
         * @brief Checks that the scene still exists and the entity handle is valid.
         * @return True when the lifetime token, scene pointer and registry handle are valid.
         * This is a current-state check; it does not transfer ownership of the scene.
         */
        bool IsValid() const
        {
            return !m_Lifetime.expired() && m_CurrentScene && m_CurrentScene->GetRegistry().valid(m_EntityHandle);
        }

        void SetParent(Entity parent);

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_CurrentScene = nullptr;
        std::weak_ptr<void> m_Lifetime;

        friend class Scene;
        friend class SceneSerializer;
        friend class ScriptableComponent;
    };

    // For entity to have children entity (need to frame this sentence correctly)
    /** @brief Intrusive hierarchy links using EnTT handles; entt::null denotes a missing link. */
    struct RelationshipComponent
    {
        entt::entity Parent = entt::null;
        entt::entity FirstChild = entt::null;
        entt::entity NextSibling = entt::null;
        entt::entity PrevSibling = entt::null;
    };
} // namespace FINALITY

#include "Entity.inl"
