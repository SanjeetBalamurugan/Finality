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

        storage.Scripts.emplace_back(std::move(script), false,
            []() -> std::unique_ptr<ScriptableComponent>
            {
                if constexpr (std::is_default_constructible_v<T>)
                    return std::make_unique<T>();
                else
                    return nullptr;
            });

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

	inline void Entity::SetParent(Entity parent)
	{
		auto& registry = m_CurrentScene->m_EntityRegistry;
		auto& rel = registry.get<RelationshipComponent>(m_EntityHandle);

        if (parent.IsValid())
        {
        if (parent.m_CurrentScene != m_CurrentScene ||
            !registry.valid(parent.GetHandle()) ||
            parent.GetHandle() == m_EntityHandle)
            return;
        entt::entity cursor = parent.GetHandle();
        size_t depth = 0;
        while (cursor != entt::null && registry.valid(cursor) && depth++ < 1024)
        {
            if (cursor == m_EntityHandle) return;
            const auto* ancestor = registry.try_get<RelationshipComponent>(cursor);
            cursor = ancestor ? ancestor->Parent : entt::null;
        }
        }
		if (rel.Parent != entt::null)
		{
			auto& oldParentRel = registry.get<RelationshipComponent>(rel.Parent);

			if (oldParentRel.FirstChild == m_EntityHandle)
			{
				oldParentRel.FirstChild = rel.NextSibling;
			}
			else if (rel.PrevSibling != entt::null)
			{
				auto& prevRel = registry.get<RelationshipComponent>(rel.PrevSibling);
				prevRel.NextSibling = rel.NextSibling;
			}

			if (rel.NextSibling != entt::null)
			{
				auto& nextRel = registry.get<RelationshipComponent>(rel.NextSibling);
				nextRel.PrevSibling = rel.PrevSibling;
			}
		}

		rel.Parent = entt::null;
		rel.NextSibling = entt::null;
		rel.PrevSibling = entt::null;

        if (!parent.IsValid()) return;
        rel.Parent = parent.GetHandle();

		auto& parentRel = registry.get<RelationshipComponent>(parent.GetHandle());

		if (parentRel.FirstChild == entt::null)
		{
			parentRel.FirstChild = m_EntityHandle;
			return;
		}

		entt::entity lastChild = parentRel.FirstChild;

		while (true)
		{
			auto& lastRel = registry.get<RelationshipComponent>(lastChild);

			if (lastRel.NextSibling == entt::null)
			{
				lastRel.NextSibling = m_EntityHandle;
				rel.PrevSibling = lastChild;
				break;
			}

			lastChild = lastRel.NextSibling;
		}
	}
}
