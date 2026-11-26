/**
 * @file
 * @ingroup scene
 * @brief Owns entities and dispatches script simulation.
 *
 * Scene owns the EnTT registry and supplies hierarchy cloning, destruction and world transforms. Script updates use a snapshot and defer structural mutations until callback dispatch completes. New attachments begin on the following update.
 *
 * @par Usage
 * Call OnUpdate with elapsed seconds. FixedUpdate runs before Update; LateUpdate follows all Update calls. Use DestroyEntity rather than removing registry entries during callbacks.
 */
#pragma once
#include <Core/Core.h>

#include <entt.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_set>
#include <functional>

namespace FINALITY
{
    class Entity;
    class ScriptableComponent;

    /**
	 * @ingroup scene
	 * @brief Owns entities and dispatches script simulation.
	 *
	 * Scene owns the EnTT registry and supplies hierarchy cloning, destruction and world transforms. Script updates use a snapshot and defer structural mutations until callback dispatch completes. New attachments begin on the following update.
	 *
	 * @par Usage
	 * Call OnUpdate with elapsed seconds. FixedUpdate runs before Update; LateUpdate follows all Update calls. Use DestroyEntity rather than removing registry entries during callbacks.
	 */
    class FAPI Scene
    {
    public:
        Scene();
        glm::mat4 GetWorldTransform(entt::entity entityHandle) const;
        virtual ~Scene();

        virtual void OnInit() = 0;
        /**
		 * @brief Dispatches activation and simulation callbacks for a scene update.
		 * @param ts Elapsed wall time in seconds, capped internally before simulation.
		 * FixedUpdate precedes Update; LateUpdate follows all Update callbacks. The fixed
		 * catch-up budget is eight steps. Structural mutations requested during dispatch
		 * are deferred to keep traversal valid. Recursive dispatch is ignored.
		 */
        virtual void OnUpdate(float ts);
        virtual void OnRender();
        virtual void OnDestroy();

        Entity CreateEntity(const std::string& name = std::string());
        Entity DuplicateEntity(entt::entity handle);
        Entity InstantiateHierarchy(const Scene& source, entt::entity handle);
        Entity GetEntity(entt::entity handle);
        void DestroyEntity(entt::entity entity);
        void CopyScriptsTo(Scene& destination) const;

        bool IsDispatchingScripts() const
        {
            return m_Dispatching;
        }

        void DeferScriptMutation(std::function<void()> operation);

        double GetScriptTimeMS() const
        {
            return m_ScriptTimeMS;
        }

        uint64_t GetScriptCalls() const
        {
            return m_ScriptCalls;
        }

        /**
         * @brief Changes the scene's simulation speed.
         * @param scale Finite multiplier clamped to the range zero through 100.
         * Non-finite values leave the current setting unchanged. Zero suppresses simulation
         * callbacks while an update can still process activation changes.
         */
        void SetTimeScale(float scale);

        float GetTimeScale() const
        {
            return m_TimeScale;
        }

        /**
         * @brief Sets the interval used for fixed simulation callbacks.
         * @param step Interval in seconds, between 0.001 and 1 inclusive.
         * Accepted values reset the accumulated fixed-step backlog. Invalid values are ignored.
         */
        void SetFixedTimeStep(float step);

        float GetFixedTimeStep() const
        {
            return m_FixedTimeStep;
        }

        uint32_t GetFixedSteps() const
        {
            return m_FixedSteps;
        }

        std::weak_ptr<void> GetLifetime() const
        {
            return m_Lifetime;
        }

        const entt::registry& GetRegistry() const
        {
            return m_EntityRegistry;
        }

        entt::registry& GetRegistry()
        {
            return m_EntityRegistry;
        }

    private:
        std::shared_ptr<void> m_Lifetime = std::make_shared<int>(0);
        entt::registry m_EntityRegistry;
        bool m_TexturesUploaded = false;
        bool m_Dispatching = false;
        std::vector<std::pair<entt::entity, std::shared_ptr<ScriptableComponent>>> m_ScriptSnapshot;
        std::unordered_set<entt::entity> m_PendingDestroy, m_Destroying;
        std::vector<std::function<void()>> m_ScriptMutations;
        double m_ScriptTimeMS = 0;
        uint64_t m_ScriptCalls = 0;
        float m_TimeScale = 1, m_FixedTimeStep = 1.0f / 60.0f;
        double m_FixedAccumulator = 0;
        uint32_t m_FixedSteps = 0;
        bool IsPendingDestroy(entt::entity entity) const;

        friend class Entity;
        friend class SceneSerializer;
        friend class ScriptableComponent;
    };

    /** @todo TODO: Document EmptyScene class semantics, ownership and usage. */
    class FAPI EmptyScene : public Scene
    {
    public:
        void OnInit() override
        {
            Scene::OnInit();
        }

        void OnUpdate(float ts) override
        {
            Scene::OnUpdate(ts);
        }

        void OnRender() override
        {
            Scene::OnRender();
        }

        void OnDestroy() override
        {
            Scene::OnDestroy();
        }
    };
} // namespace FINALITY
