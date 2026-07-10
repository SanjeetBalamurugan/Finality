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
		virtual void OnDestroy();

		Entity CreateEntity(const std::string& name = std::string());
	private:
		entt::registry m_EntityRegistry;

		friend class Entity;
	};

	class FAPI EmptyScene : public Scene
	{
	public:
		void OnInit() override {}
		void OnUpdate(float ts) override {}
		void OnDestroy() override {}
	};
}