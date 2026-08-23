#pragma once
#include <Core/Core.h>

#include <entt.hpp>

namespace FINALITY
{
	class Entity;

	class FAPI Scene
	{
	public:
		~Scene() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate(float ts);
		virtual void OnRender();
		virtual void OnDestroy();

		Entity CreateEntity(const std::string& name = std::string());
	private:
		entt::registry m_EntityRegistry;
		bool m_TexturesUploaded = false;

		friend class Entity;
		friend class ScriptableComponent;
	};

	class FAPI EmptyScene : public Scene
	{
	public:
		void OnInit() override {}
		void OnUpdate(float ts) override {}
		void OnRender() override {}
		void OnDestroy() override {}
	};
}