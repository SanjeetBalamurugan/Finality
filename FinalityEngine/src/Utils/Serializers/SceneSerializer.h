#pragma once

#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <Scene/Entity.h>
#include <entt.hpp>

#include <Core/Core.h>
#include <string>
#include <unordered_map>

namespace FINALITY
{
	class Scene;

	class FAPI SceneSerializer
	{
	private:
		static void SerializeEntity(YAML::Emitter& out, FINALITY::Entity entity, const entt::registry& registry);
		static void DeSerializeEntity(const YAML::Node& entityNode, FINALITY::Scene* scene, std::unordered_map<FINALITY::UUID, entt::entity>& entityMap);
	public:
		SceneSerializer(const Scene& scene, const std::string& name = "Untitled Scene");

		void Serialize(const std::string& filePath);
		bool DeSerialize(const std::string& filePath);
        std::string SerializeToString() const;
        bool DeSerializeFromString(const std::string& text);

	private:
		const Scene& m_Scene;
		std::string m_SceneName;
	};
}
