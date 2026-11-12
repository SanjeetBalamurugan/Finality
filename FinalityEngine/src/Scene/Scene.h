#pragma once
#include <Core/Core.h>

#include <entt.hpp>
#include <glm/glm.hpp>

namespace FINALITY
{
	class Entity;

	class FAPI Scene
	{
	public:
        Scene();
        glm::mat4 GetWorldTransform(entt::entity entityHandle) const;
		virtual ~Scene();

		virtual void OnInit() = 0;
		virtual void OnUpdate(float ts);
		virtual void OnRender();
		virtual void OnDestroy();

		Entity CreateEntity(const std::string& name = std::string());
        Entity DuplicateEntity(entt::entity handle);
        Entity InstantiateHierarchy(const Scene& source, entt::entity handle);
        Entity GetEntity(entt::entity handle);
		void DestroyEntity(entt::entity entity);
        void CopyScriptsTo(Scene& destination) const;

		const entt::registry& GetRegistry() const { return m_EntityRegistry; }
		entt::registry& GetRegistry() { return m_EntityRegistry; }
	private:
		entt::registry m_EntityRegistry;
		bool m_TexturesUploaded = false;

		friend class Entity;
		friend class SceneSerializer;
		friend class ScriptableComponent;
	};

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
}
