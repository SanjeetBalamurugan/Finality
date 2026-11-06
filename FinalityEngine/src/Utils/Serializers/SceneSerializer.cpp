#include "SceneSerializer.h"
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <fstream>
#include <sstream>
#include <Core/Log.h>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <Assets/AssetManager.h>
#include <Assets/MaterialAsset.h>
#include <Renderer/Renderer.h>
#include <Assets/AssetManager.h>
#include <Assets/MaterialAsset.h>
#include <Renderer/Renderer.h>

namespace YAML
{
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};
}

static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

FINALITY::SceneSerializer::SceneSerializer(const Scene& scene, const std::string& name)
	: m_Scene(scene), m_SceneName(name)
{}

void FINALITY::SceneSerializer::SerializeEntity(
	YAML::Emitter& out,
	FINALITY::Entity entity,
	const entt::registry& registry)
{
	out << YAML::BeginMap;

	if (entity.HasComponent<FINALITY::TagComponent>())
	{
		auto& tag = entity.GetComponent<FINALITY::TagComponent>();

		out << YAML::Key << "Entity";
		out << YAML::Value << static_cast<uint64_t>(tag.uuid);

		out << YAML::Key << "TagComponent";
		out << YAML::BeginMap;

		out << YAML::Key << "Tag";
		out << YAML::Value << tag.tag;

		out << YAML::EndMap;
	}

	if (entity.HasComponent<FINALITY::RelationshipComponent>())
	{
		auto& relationship = entity.GetComponent<FINALITY::RelationshipComponent>();

		auto GetUUID = [&](entt::entity e) -> uint64_t
		{
			if (e == entt::null || !registry.valid(e))
				return 0;

			if (!registry.all_of<FINALITY::TagComponent>(e))
				return 0;

			return static_cast<uint64_t>(
				registry.get<FINALITY::TagComponent>(e).uuid
				);
		};

		out << YAML::Key << "RelationshipComponent";
		out << YAML::BeginMap;

		out << YAML::Key << "Parent";
		out << YAML::Value << GetUUID(relationship.Parent);

		out << YAML::Key << "FirstChild";
		out << YAML::Value << GetUUID(relationship.FirstChild);

		out << YAML::Key << "NextSibling";
		out << YAML::Value << GetUUID(relationship.NextSibling);

		out << YAML::Key << "PrevSibling";
		out << YAML::Value << GetUUID(relationship.PrevSibling);

		out << YAML::EndMap;
	}

	if (entity.HasComponent<FINALITY::TransformComponent>())
	{
		auto& tc = entity.GetComponent<FINALITY::TransformComponent>();

		out << YAML::Key << "TransformComponent";
		out << YAML::BeginMap;

		out << YAML::Key << "Position" << YAML::Value << tc.Position;
		out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
		out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

		out << YAML::EndMap;
	}

	if (entity.HasComponent<FINALITY::MeshComponent>())
	{
		auto& mc = entity.GetComponent<FINALITY::MeshComponent>();

		out << YAML::Key << "MeshComponent";
		out << YAML::BeginMap;

		out << YAML::Key << "MeshIdentifier" << YAML::Value << mc.MeshIdentifier;

		out << YAML::EndMap;
	}

	if (entity.HasComponent<FINALITY::MaterialComponent>())
	{
		const auto& material = entity.GetComponent<FINALITY::MaterialComponent>();
		out << YAML::Key << "MaterialComponent";
		out << YAML::BeginMap;
		out << YAML::Key << "MaterialIdentifier" << YAML::Value << material.MaterialIdentifier;
		if (material.MaterialInstance)
        {
            out << YAML::Key << "Overrides" << YAML::Value << YAML::BeginSeq;
            std::vector<std::string> names;
            for (const auto& entry : material.MaterialInstance->GetUniforms())
                names.push_back(entry.first);
            std::sort(names.begin(), names.end());
            for (const auto& name : names)
            {
                const auto& uniform = material.MaterialInstance->GetUniforms().at(name);
                const size_t size = uniform.Size;
                const char* type = size == sizeof(float) ? "Float" :
                    size == sizeof(glm::vec4) ? "Vec4" :
                    size == sizeof(glm::mat4) ? "Mat4" : nullptr;
                if (!type) continue;
                out << YAML::BeginMap;
                out << YAML::Key << "Name" << YAML::Value << name;
                out << YAML::Key << "Type" << YAML::Value << type;
                out << YAML::Key << "Value" << YAML::Value;
                if (size == sizeof(float))
                {
                    float v = 0.0f;
                    material.MaterialInstance->TryGetUniform1f(name, v);
                    out << v;
                }
                else
                {
                    out << YAML::Flow << YAML::BeginSeq;
                    if (size == sizeof(glm::vec4))
                    {
                        glm::vec4 v(0.0f);
                        material.MaterialInstance->TryGetUniformVec4(name, v);
                        for (int i = 0; i < 4; ++i) out << v[i];
                    }
                    else
                    {
                        glm::mat4 v(1.0f);
                        material.MaterialInstance->TryGetUniformMat4(name, v);
                        for (int c = 0; c < 4; ++c)
                            for (int r = 0; r < 4; ++r) out << v[c][r];
                    }
                    out << YAML::EndSeq;
                }
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }
        out << YAML::EndMap;
	}

	if (entity.HasComponent<FINALITY::CameraComponent>())
	{
		auto& cc = entity.GetComponent<FINALITY::CameraComponent>();

        out << YAML::Key << "CameraComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "IsPrimary" << YAML::Value << cc.IsPrimary;
        out << YAML::Key << "FOV" << YAML::Value << cc.FOV;
        out << YAML::Key << "AspectRatio" << YAML::Value << cc.AspectRatio;
        out << YAML::Key << "NearClip" << YAML::Value << cc.NearClip;
        out << YAML::Key << "FarClip" << YAML::Value << cc.FarClip;
		out << YAML::EndMap;
	}

	out << YAML::EndMap;
}

void FINALITY::SceneSerializer::DeSerializeEntity(
	const YAML::Node& entityNode,
	FINALITY::Scene* scene,
	std::unordered_map<FINALITY::UUID, entt::entity>& entityMap)
{
	auto& registry = scene->m_EntityRegistry;

	FINALITY::UUID uuid(entityNode["Entity"].as<uint64_t>());

	entt::entity enttEntity = registry.create();
	FINALITY::Entity entity(enttEntity, scene);

	if (auto tagNode = entityNode["TagComponent"])
	{
		auto& tag = entity.AddComponent<FINALITY::TagComponent>();

		tag.uuid = uuid;
		tag.tag = tagNode["Tag"].as<std::string>();
	}

	if (entityNode["RelationshipComponent"])
		entity.AddComponent<FINALITY::RelationshipComponent>();

	if (auto transformNode = entityNode["TransformComponent"])
	{
		auto& tc = entity.AddComponent<FINALITY::TransformComponent>();

		tc.Position = transformNode["Position"].as<glm::vec3>();
		tc.Rotation = transformNode["Rotation"].as<glm::vec3>();
		tc.Scale = transformNode["Scale"].as<glm::vec3>();
		tc.Matrix = tc.GetTransformMatrix();
	}

	if (auto meshNode = entityNode["MeshComponent"])
	{
		auto& mc = entity.AddComponent<FINALITY::MeshComponent>();

		if (meshNode["MeshIdentifier"])
		{
			mc.MeshIdentifier = meshNode["MeshIdentifier"].as<std::string>();
			if (!mc.MeshIdentifier.empty())
				mc.MeshData = FINALITY::Mesh::Create(mc.MeshIdentifier);
		}
	}

	if (auto materialNode = entityNode["MaterialComponent"])
    {
        auto& component = entity.AddComponent<FINALITY::MaterialComponent>();
        if (materialNode["MaterialIdentifier"])
        {
            component.MaterialIdentifier = materialNode["MaterialIdentifier"].as<std::string>();
            if (!component.MaterialIdentifier.empty() && FINALITY::Renderer::GetDevice())
            {
                auto asset = FINALITY::AssetManager::CreateAsset<FINALITY::MaterialAsset>(
                    component.MaterialIdentifier, FINALITY::Renderer::GetDevice());
                if (asset) component.MaterialInstance = asset->CreateInstance();
            }
        }
        if (component.MaterialInstance && materialNode["Overrides"])
        {
            for (const auto& property : materialNode["Overrides"])
            {
                const std::string name = property["Name"].as<std::string>();
                const std::string type = property["Type"].as<std::string>();
                const auto values = property["Value"];
                if (type == "Float")
                    component.MaterialInstance->SetUniform1f(name, values.as<float>());
                else if (type == "Vec4" && values.IsSequence() && values.size() == 4)
                {
                    glm::vec4 v(0.0f);
                    for (int i = 0; i < 4; ++i) v[i] = values[i].as<float>();
                    component.MaterialInstance->SetUniformVec4(name, v);
                }
                else if (type == "Mat4" && values.IsSequence() && values.size() == 16)
                {
                    glm::mat4 m(1.0f);
                    for (int c = 0; c < 4; ++c)
                        for (int r = 0; r < 4; ++r)
                            m[c][r] = values[c * 4 + r].as<float>();
                    component.MaterialInstance->SetUniformMat4(name, m);
                }
            }
        }
    }

    if (auto cameraNode = entityNode["CameraComponent"])
	{
        if (cameraNode["CameraComponent"])
            cameraNode = cameraNode["CameraComponent"];
		float fov = cameraNode["FOV"] ? cameraNode["FOV"].as<float>() : 45.0f;
		float aspect = cameraNode["AspectRatio"] ? cameraNode["AspectRatio"].as<float>() : 1.778f;
		float nearClip = cameraNode["NearClip"] ? cameraNode["NearClip"].as<float>() : 0.1f;
		float farClip = cameraNode["FarClip"] ? cameraNode["FarClip"].as<float>() : 1000.0f;

		auto& cc = entity.AddComponent<FINALITY::CameraComponent>(fov, aspect, nearClip, farClip);
		if (cameraNode["IsPrimary"])
			cc.IsPrimary = cameraNode["IsPrimary"].as<bool>();
	}

	entityMap[uuid] = enttEntity;
}

bool FINALITY::SceneSerializer::DeSerialize(const std::string& filePath)
{
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream)
    {
        FI_CORE_ERROR("Failed to open scene file {}", filePath);
        return false;
    }
    std::stringstream contents;
    contents << stream.rdbuf();
    return DeSerializeFromString(contents.str());
}

bool FINALITY::SceneSerializer::DeSerializeFromString(const std::string& contents)
{
    try
    {
    YAML::Node data = YAML::Load(contents);
    if (!data["Scene"])
        throw std::runtime_error("Scene root is missing");
	auto entities = data["Entities"];

	if (!entities)
		return false;

	auto* scene = const_cast<FINALITY::Scene*>(&m_Scene);
	auto& registry = scene->m_EntityRegistry;
    if (registry.view<TagComponent>().begin() != registry.view<TagComponent>().end())
        throw std::runtime_error("Deserialize into a new, empty scene");

	std::unordered_map<FINALITY::UUID, entt::entity> entityMap;

	for (const auto& entityNode : entities)
		DeSerializeEntity(entityNode, scene, entityMap);

	for (const auto& entityNode : entities)
	{
		FINALITY::UUID uuid(entityNode["Entity"].as<uint64_t>());

		auto entityIt = entityMap.find(uuid);

		if (entityIt == entityMap.end())
			continue;

		FINALITY::Entity entity(entityIt->second, scene);

		auto relationshipNode = entityNode["RelationshipComponent"];

		if (!relationshipNode)
			continue;

		auto& relationship =
			entity.GetComponent<FINALITY::RelationshipComponent>();

		auto ResolveEntity = [&](const char* key) -> entt::entity
		{
			if (!relationshipNode[key])
				return entt::null;

			FINALITY::UUID targetUUID(
				relationshipNode[key].as<uint64_t>()
			);

			if (static_cast<uint64_t>(targetUUID) == 0)
				return entt::null;

			auto targetIt = entityMap.find(targetUUID);

			if (targetIt == entityMap.end())
				return entt::null;

			return targetIt->second;
		};

		relationship.Parent = ResolveEntity("Parent");
		relationship.FirstChild = ResolveEntity("FirstChild");
		relationship.NextSibling = ResolveEntity("NextSibling");
		relationship.PrevSibling = ResolveEntity("PrevSibling");
	}

    return true;
    }
    catch (const std::exception& e)
    {
        FI_CORE_ERROR("Scene deserialization failed: {}", e.what());
        return false;
    }
}

std::string FINALITY::SceneSerializer::SerializeToString() const
{
	YAML::Emitter output;

	output << YAML::BeginMap;

	output << YAML::Key << "Scene";
	output << YAML::Value << m_SceneName;

	output << YAML::Key << "Entities";
	output << YAML::Value << YAML::BeginSeq;

	auto eView = m_Scene.m_EntityRegistry.view<TagComponent>();

	for (auto entity : eView)
	{
		FINALITY::Entity temp(
			entity,
			const_cast<FINALITY::Scene*>(&m_Scene)
		);

		SerializeEntity(
			output,
			temp,
			m_Scene.m_EntityRegistry
		);
	}

	output << YAML::EndSeq;
	output << YAML::EndMap;

    return std::string(output.c_str());
}

void FINALITY::SceneSerializer::Serialize(const std::string& filePath)
{
    std::ofstream stream(filePath, std::ios::binary);
    if (stream) stream << SerializeToString();
    else FI_CORE_ERROR("Failed to save scene file {}", filePath);
}
