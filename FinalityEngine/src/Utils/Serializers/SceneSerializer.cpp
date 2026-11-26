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
#include <Scene/ScriptRegistry.h>

namespace YAML
{
    template <>
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
            {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };
} // namespace YAML

static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

FINALITY::SceneSerializer::SceneSerializer(const Scene& scene, const std::string& name)
    : m_Scene(scene), m_SceneName(name)
{
}

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
            {
                return 0;
            }

            if (!registry.all_of<FINALITY::TagComponent>(e))
            {
                return 0;
            }

            return static_cast<uint64_t>(
                registry.get<FINALITY::TagComponent>(e).uuid);
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
        out << YAML::Key << "Basis" << YAML::Value << YAML::Flow << YAML::BeginSeq;
        for (int c = 0; c < 4; ++c)
        {
            for (int r = 0; r < 4; ++r)
            {
                out << tc.Basis[c][r];
            }
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;
    }

    if (entity.HasComponent<FINALITY::MeshComponent>())
    {
        auto& mc = entity.GetComponent<FINALITY::MeshComponent>();

        out << YAML::Key << "MeshComponent";
        out << YAML::BeginMap;

        out << YAML::Key << "MeshIdentifier" << YAML::Value << mc.MeshIdentifier;
        out << YAML::Key << "Occluder" << YAML::Value << mc.Occluder;

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
            out << YAML::Key << "Textures" << YAML::Value << YAML::BeginSeq;
            for (const auto& [name, texture] : material.MaterialInstance->GetTextures())
            {
                if (!texture)
                {
                    continue;
                }
                const auto* record = AssetRegistry::Find(static_cast<uint64_t>(texture->Handle));
                if (!record)
                {
                    continue;
                }
                out << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << name;
                out << YAML::Key << "Asset" << YAML::Value << "asset://" + std::to_string(record->ID) << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }
        if (material.MaterialInstance)
        {
            out << YAML::Key << "Overrides" << YAML::Value << YAML::BeginSeq;
            std::vector<std::string> names;
            for (const auto& entry : material.MaterialInstance->GetUniforms())
            {
                names.push_back(entry.first);
            }
            std::sort(names.begin(), names.end());
            for (const auto& name : names)
            {
                const auto& uniform = material.MaterialInstance->GetUniforms().at(name);
                const size_t size = uniform.Size;
                const char* type = size == sizeof(float) ? "Float" : size == sizeof(glm::vec4) ? "Vec4"
                                                                 : size == sizeof(glm::mat4)   ? "Mat4"
                                                                                               : nullptr;
                if (!type)
                {
                    continue;
                }
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
                        for (int i = 0; i < 4; ++i)
                        {
                            out << v[i];
                        }
                    }
                    else
                    {
                        glm::mat4 v(1.0f);
                        material.MaterialInstance->TryGetUniformMat4(name, v);
                        for (int c = 0; c < 4; ++c)
                        {
                            for (int r = 0; r < 4; ++r)
                            {
                                out << v[c][r];
                            }
                        }
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
        out << YAML::Key << "OcclusionCulling" << YAML::Value << cc.OcclusionCulling;
        out << YAML::Key << "FOV" << YAML::Value << cc.FOV;
        out << YAML::Key << "AspectRatio" << YAML::Value << cc.AspectRatio;
        out << YAML::Key << "NearClip" << YAML::Value << cc.NearClip;
        out << YAML::Key << "FarClip" << YAML::Value << cc.FarClip;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<LODGroupComponent>())
    {
        const auto& group = entity.GetComponent<LODGroupComponent>();
        out << YAML::Key << "LODGroupComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << group.Enabled;
        out << YAML::Key << "AutoBounds" << YAML::Value << group.AutoBounds;
        out << YAML::Key << "LocalCenter" << YAML::Value << group.LocalCenter;
        out << YAML::Key << "LocalSize" << YAML::Value << group.LocalSize;
        out << YAML::Key << "Hysteresis" << YAML::Value << group.Hysteresis;
        out << YAML::Key << "Levels" << YAML::Value << YAML::BeginSeq;
        for (const auto& level : group.Levels)
        {
            out << YAML::BeginMap << YAML::Key << "ScreenRelativeHeight" << YAML::Value << level.ScreenRelativeHeight;
            out << YAML::Key << "Renderers" << YAML::Value << YAML::Flow << YAML::BeginSeq;
            for (auto id : level.Renderers)
            {
                out << id;
            }
            out << YAML::EndSeq << YAML::EndMap;
        }
        out << YAML::EndSeq << YAML::EndMap;
    }

    if (entity.HasComponent<ScriptStorage>())
    {
        out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
        for (const auto& script : entity.GetComponent<ScriptStorage>().Scripts)
        {
            if (script.TypeName.empty())
            {
                continue;
            }
            auto properties = script.Properties;
            if (const auto* definition = ScriptRegistry::Find(script.TypeName); definition && script.Instance)
            {
                for (const auto& property : definition->Properties)
                {
                    properties[property.Name] = property.Get(*script.Instance);
                }
            }
            out << YAML::BeginMap << YAML::Key << "Type" << YAML::Value << script.TypeName;
            out << YAML::Key << "Enabled" << YAML::Value << script.Enabled;
            out << YAML::Key << "Properties" << YAML::Value << YAML::BeginSeq;
            for (const auto& [name, value] : properties)
            {
                out << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << name;
                out << YAML::Key << "Kind" << YAML::Value << value.index();
                out << YAML::Key << "Value" << YAML::Value;
                std::visit([&](const auto& item)
                           {
                               using T = std::decay_t<decltype(item)>;
                               if constexpr (std::is_same_v<T, ScriptAssetReference>)
                               {
                                   out << YAML::BeginMap << YAML::Key << "AssetType" << YAML::Value << static_cast<int>(item.Type);
                                   out << YAML::Key << "Reference" << YAML::Value << item.Reference << YAML::EndMap;
                               }
                               else
                               {
                                   out << item;
                               }
                           },
                           value);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq << YAML::EndMap;
        }
        out << YAML::EndSeq;
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
    else
    {
        auto& tag = entity.AddComponent<TagComponent>("Entity");
        tag.uuid = uuid;
    }

    entity.AddComponent<FINALITY::RelationshipComponent>();

    if (auto transformNode = entityNode["TransformComponent"])
    {
        auto& tc = entity.AddComponent<FINALITY::TransformComponent>();

        tc.Position = transformNode["Position"].as<glm::vec3>();
        tc.Rotation = transformNode["Rotation"].as<glm::vec3>();
        tc.Scale = transformNode["Scale"].as<glm::vec3>();
        if (const auto basis = transformNode["Basis"])
        {
            if (!basis.IsSequence() || basis.size() != 16)
            {
                throw std::runtime_error("Invalid transform basis");
            }
            for (int c = 0; c < 4; ++c)
            {
                for (int r = 0; r < 4; ++r)
                {
                    tc.Basis[c][r] = basis[c * 4 + r].as<float>();
                }
            }
        }
        tc.Matrix = tc.GetTransformMatrix();
    }
    else
    {
        entity.AddComponent<TransformComponent>();
    }

    if (auto meshNode = entityNode["MeshComponent"])
    {
        auto& mc = entity.AddComponent<FINALITY::MeshComponent>();
        mc.Occluder = meshNode["Occluder"].as<bool>(false);

        if (meshNode["MeshIdentifier"])
        {
            mc.MeshIdentifier = meshNode["MeshIdentifier"].as<std::string>();
            if (!mc.MeshIdentifier.empty())
            {
                mc.MeshData = FINALITY::Mesh::Create(mc.MeshIdentifier);
            }
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
                auto asset = FINALITY::AssetManager::CreateAsset<FINALITY::MaterialAsset>( component.MaterialIdentifier, FINALITY::Renderer::GetDevice());
                if (asset)
                {
                    component.MaterialInstance = asset->CreateInstance();
                }
            }
        }
        if (component.MaterialInstance && materialNode["Textures"])
        {
            for (const auto& texture : materialNode["Textures"])
            {
                const auto reference = texture["Asset"].as<std::string>();
                if (auto loaded = AssetManager::CreateAsset<Texture>(reference))
                {
                    component.MaterialInstance->SetTexture(texture["Name"].as<std::string>(), loaded);
                }
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
                {
                    component.MaterialInstance->SetUniform1f(name, values.as<float>());
                }
                else if (type == "Vec4" && values.IsSequence() && values.size() == 4)
                {
                    glm::vec4 v(0.0f);
                    for (int i = 0; i < 4; ++i)
                    {
                        v[i] = values[i].as<float>();
                    }
                    component.MaterialInstance->SetUniformVec4(name, v);
                }
                else if (type == "Mat4" && values.IsSequence() && values.size() == 16)
                {
                    glm::mat4 m(1.0f);
                    for (int c = 0; c < 4; ++c)
                    {
                        for (int r = 0; r < 4; ++r)
                        {
                            m[c][r] = values[c * 4 + r].as<float>();
                        }
                    }
                    component.MaterialInstance->SetUniformMat4(name, m);
                }
            }
        }
    }

    if (auto cameraNode = entityNode["CameraComponent"])
    {
        if (cameraNode["CameraComponent"])
        {
            cameraNode = cameraNode["CameraComponent"];
        }
        float fov = cameraNode["FOV"] ? cameraNode["FOV"].as<float>() : 45.0f;
        float aspect = cameraNode["AspectRatio"] ? cameraNode["AspectRatio"].as<float>() : 1.778f;
        float nearClip = cameraNode["NearClip"] ? cameraNode["NearClip"].as<float>() : 0.1f;
        float farClip = cameraNode["FarClip"] ? cameraNode["FarClip"].as<float>() : 1000.0f;

        auto& cc = entity.AddComponent<FINALITY::CameraComponent>(fov, aspect, nearClip, farClip);
        cc.OcclusionCulling = cameraNode["OcclusionCulling"].as<bool>(false);
        cc.CameraInstance->SetOcclusionCulling(cc.OcclusionCulling);
        if (cameraNode["IsPrimary"])
        {
            cc.IsPrimary = cameraNode["IsPrimary"].as<bool>();
        }
    }

    if (const auto node = entityNode["LODGroupComponent"])
    {
        auto& group = entity.AddComponent<LODGroupComponent>();
        group.Enabled = node["Enabled"].as<bool>(true);
        group.AutoBounds = node["AutoBounds"].as<bool>(true);
        if (node["LocalCenter"])
        {
            group.LocalCenter = node["LocalCenter"].as<glm::vec3>();
        }
        group.LocalSize = node["LocalSize"].as<float>(1);
        group.Hysteresis = node["Hysteresis"].as<float>(0.05f);
        if (const auto levels = node["Levels"]; levels && levels.IsSequence())
        {
            group.Levels.clear();
            for (const auto& level : levels)
            {
                if (group.Levels.size() >= 8)
                {
                    break;
                }
                LODLevel entry;
                entry.ScreenRelativeHeight = level["ScreenRelativeHeight"].as<float>(0.1f);
                if (const auto renderers = level["Renderers"]; renderers && renderers.IsSequence())
                {
                    for (const auto& renderer : renderers)
                    {
                        entry.Renderers.push_back(renderer.as<uint64_t>());
                    }
                }
                group.Levels.push_back(std::move(entry));
            }
        }
        NormalizeLODGroup(group);
    }

    if (entityNode["Scripts"])
    {
        for (const auto& node : entityNode["Scripts"])
        {
            const auto name = node["Type"].as<std::string>();
            if (!ScriptRegistry::Attach(*scene, enttEntity, name))
            {
                continue;
            }
            auto& script = registry.get<ScriptStorage>(enttEntity).Scripts.back();
            script.Enabled = node["Enabled"].as<bool>(true);
            if (node["Properties"])
            {
                for (const auto& property : node["Properties"])
                {
                    ScriptValue value;
                    switch (property["Kind"].as<int>())
                    {
                    case 0:
                        value = property["Value"].as<bool>();
                        break;
                    case 1:
                        value = property["Value"].as<int>();
                        break;
                    case 2:
                        value = property["Value"].as<float>();
                        break;
                    case 3:
                        value = property["Value"].as<std::string>();
                        break;
                    case 4:
                        value = property["Value"].as<glm::vec3>();
                        break;
                    case 5:
                    {
                        const int type = property["Value"]["AssetType"].as<int>();
                        if (type < 0 || type > static_cast<int>(ProjectAssetType::Prefab))
                        {
                            throw std::runtime_error("Invalid script asset type");
                        }
                        value = ScriptAssetReference{ static_cast<ProjectAssetType>(type), property["Value"]["Reference"].as<std::string>() };
                        break;
                    }
                    default:
                        throw std::runtime_error("Unsupported script property type");
                    }
                    script.Properties[property["Name"].as<std::string>()] = value;
                }
            }
            if (const auto* definition = ScriptRegistry::Find(name); definition && script.Instance)
            {
                for (const auto& property : definition->Properties)
                {
                    if (const auto found = script.Properties.find(property.Name); found != script.Properties.end())
                    {
                        if (property.Get(*script.Instance).index() == found->second.index())
                        {
                            property.Set(*script.Instance, found->second);
                        }
                    }
                }
            }
        }
    }
    if (entityMap.contains(uuid))
    {
        throw std::runtime_error("Duplicate entity UUID");
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
        {
            throw std::runtime_error("Scene root is missing");
        }
        auto entities = data["Entities"];

        if (!entities)
        {
            return false;
        }

        auto* scene = const_cast<FINALITY::Scene*>(&m_Scene);
        auto& registry = scene->m_EntityRegistry;
        if (registry.view<TagComponent>().begin() != registry.view<TagComponent>().end())
        {
            throw std::runtime_error("Deserialize into a new, empty scene");
        }

        std::unordered_map<FINALITY::UUID, entt::entity> entityMap;

        for (const auto& entityNode : entities)
        {
            DeSerializeEntity(entityNode, scene, entityMap);
        }

        for (const auto& entityNode : entities)
        {
            FINALITY::UUID uuid(entityNode["Entity"].as<uint64_t>());

            auto entityIt = entityMap.find(uuid);

            if (entityIt == entityMap.end())
            {
                continue;
            }

            FINALITY::Entity entity(entityIt->second, scene);

            auto relationshipNode = entityNode["RelationshipComponent"];

            if (!relationshipNode)
            {
                continue;
            }

            auto& relationship = entity.GetComponent<FINALITY::RelationshipComponent>();

            auto ResolveEntity = [&](const char* key) -> entt::entity
            {
                if (!relationshipNode[key])
                {
                    return entt::null;
                }

                FINALITY::UUID targetUUID(
                    relationshipNode[key].as<uint64_t>());

                if (static_cast<uint64_t>(targetUUID) == 0)
                {
                    return entt::null;
                }

                auto targetIt = entityMap.find(targetUUID);

                if (targetIt == entityMap.end())
                {
                    return entt::null;
                }

                return targetIt->second;
            };

            const auto parent = ResolveEntity("Parent");
            entity.SetParent(scene->GetEntity(parent));
            if (relationship.Parent != parent)
            {
                throw std::runtime_error("Scene hierarchy contains a cycle");
            }
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
            const_cast<FINALITY::Scene*>(&m_Scene));

        SerializeEntity(
            output,
            temp,
            m_Scene.m_EntityRegistry);
    }

    output << YAML::EndSeq;
    output << YAML::EndMap;

    return std::string(output.c_str());
}

void FINALITY::SceneSerializer::Serialize(const std::string& filePath)
{
    std::ofstream stream(filePath, std::ios::binary);
    if (stream)
    {
        stream << SerializeToString();
    }
    else
    {
        FI_CORE_ERROR("Failed to save scene file {}", filePath);
    }
}
