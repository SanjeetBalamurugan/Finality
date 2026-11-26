/**
 * @file
 * @ingroup scene
 * @brief Shares hierarchy world-transform evaluation within a traversal.
 *
 * World-transform helpers compose local transforms through parents and allow a traversal to reuse computed matrices. The cache is scoped to the work that owns it; it is not a persistent dirty-transform system.
 *
 * @par Usage
 * Refresh traversal caches when scene transforms can change. Avoid treating cached matrices as authoritative across arbitrary simulation updates.
 */
#pragma once
#include "Components.h"
#include "Entity.h"
#include <unordered_map>
#include <vector>

namespace FINALITY
{
    /** @todo TODO: Document WorldTransforms class semantics, ownership and usage. */
    class WorldTransforms
    {
        const entt::registry& m_Registry;
        std::unordered_map<entt::entity, glm::mat4> m_Values;
        std::vector<entt::entity> m_Path;

    public:
        explicit WorldTransforms(const entt::registry& registry) : m_Registry(registry)
        {
            m_Path.reserve(32);
        }

        glm::mat4 Get(entt::entity entity)
        {
            const auto* local = m_Registry.try_get<TransformComponent>(entity);
            if (!local)
            {
                return glm::mat4(1);
            }
            const auto* parent = m_Registry.try_get<RelationshipComponent>(entity);
            if (!parent || !m_Registry.valid(parent->Parent))
            {
                return local->GetTransformMatrix();
            }
            m_Path.clear();
            auto cursor = entity;
            glm::mat4 value(1);
            while (m_Registry.valid(cursor) && m_Path.size() < 1024)
            {
                if (const auto known = m_Values.find(cursor); known != m_Values.end())
                {
                    value = known->second;
                    break;
                }
                if (!m_Registry.all_of<TransformComponent>(cursor))
                {
                    break;
                }
                m_Path.push_back(cursor);
                const auto* relationship = m_Registry.try_get<RelationshipComponent>(cursor);
                cursor = relationship ? relationship->Parent : entt::null;
            }
            for (auto it = m_Path.rbegin(); it != m_Path.rend(); ++it)
            {
                value *= m_Registry.get<TransformComponent>(*it).GetTransformMatrix();
                m_Values.insert_or_assign(*it, value);
            }
            return value;
        }
    };
} // namespace FINALITY
