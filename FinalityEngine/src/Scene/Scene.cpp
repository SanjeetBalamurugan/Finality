#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Renderer/Renderer.h"
#include <unordered_map>
#include <chrono>
#include <glm/gtc/quaternion.hpp>
#include <Scene/ScriptRegistry.h>
#include "WorldTransforms.h"

namespace FINALITY
{
    Scene::Scene() = default;
    Scene::~Scene() = default;

    glm::mat4 Scene::GetWorldTransform(entt::entity entityHandle) const
    {
        auto* transform = m_EntityRegistry.try_get<TransformComponent>(entityHandle);
        if (!transform)
        {
            return glm::mat4(1.0f);
        }

        glm::mat4 worldMatrix = transform->GetTransformMatrix();

        auto* rel = m_EntityRegistry.try_get<RelationshipComponent>(entityHandle);
        entt::entity parentHandle = rel ? rel->Parent : entt::null;

        size_t hops = 0;
        while (parentHandle != entt::null && m_EntityRegistry.valid(parentHandle) && hops++ < 1024)
        {
            auto* parentTransform = m_EntityRegistry.try_get<TransformComponent>(parentHandle);
            if (!parentTransform)
            {
                break;
            }

            worldMatrix = parentTransform->GetTransformMatrix() * worldMatrix;

            auto* parentRel = m_EntityRegistry.try_get<RelationshipComponent>(parentHandle);
            parentHandle = parentRel ? parentRel->Parent : entt::null;
        }

        return worldMatrix;
    }

    void Scene::OnInit()
    {
        Renderer::GetDevice()->BeginTextureBatch();
        m_TexturesUploaded = false;
    }

    bool Scene::IsPendingDestroy(entt::entity entity) const
    {
        for (size_t depth = 0; entity != entt::null && m_EntityRegistry.valid(entity) && depth < 1024; ++depth)
        {
            if (m_PendingDestroy.contains(entity))
            {
                return true;
            }
            const auto* relationship = m_EntityRegistry.try_get<RelationshipComponent>(entity);
            entity = relationship ? relationship->Parent : entt::null;
        }
        return false;
    }

    void Scene::DeferScriptMutation(std::function<void()> operation)
    {
        if (m_Dispatching)
        {
            m_ScriptMutations.push_back(std::move(operation));
        }
        else
        {
            operation();
        }
    }

    void Scene::OnRender()
    {
        Renderer::SetActiveCamera(nullptr);
        if (!m_TexturesUploaded && Renderer::GetDevice()->IsUploadBatchActive())
        {
            Renderer::GetDevice()->EndAndSubmitTextureBatch();
            m_TexturesUploaded = true;
        }

        WorldTransforms transforms(m_EntityRegistry);
        auto cameraView = m_EntityRegistry.view<CameraComponent, TransformComponent>();
        for (auto entityHandle : cameraView)
        {
            auto& cam = cameraView.get<CameraComponent>(entityHandle);
            auto& transform = cameraView.get<TransformComponent>(entityHandle);
            (void)transform;

            if (cam.IsPrimary && cam.CameraInstance)
            {
                const glm::mat4 world = transforms.Get(entityHandle);
                cam.CameraInstance->SetPosition(glm::vec3(world[3]));
                cam.CameraInstance->SetOcclusionCulling(cam.OcclusionCulling);
                auto orientation = glm::quat_cast(glm::mat3(world));
                cam.CameraInstance->SetRotation(glm::degrees(glm::eulerAngles(orientation)));
                Renderer::SetActiveCamera(cam.CameraInstance.get());
            }
        }

        Renderer::BeginScene();

        auto renderView = m_EntityRegistry.view<MeshComponent>();
        for (auto entityHandle : renderView)
        {
            Entity entity{ entityHandle, this };
            glm::mat4 worldTransform = transforms.Get(entityHandle);
            Renderer::PushEntity(entity, worldTransform);
        }

        Renderer::ConfigureLODGroups(*this);
        Renderer::EndScene();
    }

    void Scene::OnDestroy()
    {
        std::vector<std::pair<std::shared_ptr<ScriptableComponent>, bool>> instances;
        for (auto entityHandle : m_EntityRegistry.view<ScriptStorage>())
        {
            for (auto& script : m_EntityRegistry.get<ScriptStorage>(entityHandle).Scripts)
            {
                if (script.Instance && script.Awakened)
                {
                    script.Awakened = false;
                    instances.emplace_back(script.Instance, script.Active);
                    script.Active = false;
                }
            }
        }
        for (const auto& [instance, active] : instances)
        {
            if (active)
            {
                instance->OnDisable();
            }
            instance->OnDestroy();
        }

        if (Renderer::GetDevice())
        {
            Renderer::GetDevice()->WaitIdle();
        }

        m_EntityRegistry.clear();
    }

    Entity Scene::CreateEntity(const std::string& name)
    {
        Entity entity{ m_EntityRegistry.create(), this };

        entity.AddComponent<TagComponent>().tag =
            name.empty() ? "Empty Entity" : name;

        entity.AddComponent<TransformComponent>();

        auto& rel = entity.AddComponent<RelationshipComponent>();
        rel.Parent = entt::null;
        rel.FirstChild = entt::null;
        rel.NextSibling = entt::null;
        rel.PrevSibling = entt::null;

        return entity;
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (!m_EntityRegistry.valid(entity) || m_Destroying.contains(entity))
        {
            return;
        }
        if (m_Dispatching)
        {
            m_PendingDestroy.insert(entity);
            return;
        }
        m_Destroying.insert(entity);

        struct Release
        {
            std::unordered_set<entt::entity>& Set;
            entt::entity Entity;

            ~Release()
            {
                Set.erase(Entity);
            }
        } release{ m_Destroying, entity };

        if (Renderer::GetDevice())
        {
            Renderer::GetDevice()->WaitIdle();
        }
        if (auto* rel = m_EntityRegistry.try_get<RelationshipComponent>(entity))
        {
            while (auto* children = m_EntityRegistry.try_get<RelationshipComponent>(entity))
            {
                const auto child = children->FirstChild;
                if (!m_EntityRegistry.valid(child) || m_Destroying.contains(child))
                {
                    break;
                }
                DestroyEntity(child);
            }
            rel = m_EntityRegistry.try_get<RelationshipComponent>(entity);
            if (!rel)
            {
                return;
            }
            if (rel->Parent != entt::null && m_EntityRegistry.valid(rel->Parent))
            {
                if (auto* parent = m_EntityRegistry.try_get<RelationshipComponent>(rel->Parent))
                {
                    if (parent->FirstChild == entity)
                    {
                        parent->FirstChild = rel->NextSibling;
                    }
                }
            }
            if (rel->PrevSibling != entt::null && m_EntityRegistry.valid(rel->PrevSibling))
            {
                if (auto* previous = m_EntityRegistry.try_get<RelationshipComponent>(rel->PrevSibling))
                {
                    previous->NextSibling = rel->NextSibling;
                }
            }
            if (rel->NextSibling != entt::null && m_EntityRegistry.valid(rel->NextSibling))
            {
                if (auto* next = m_EntityRegistry.try_get<RelationshipComponent>(rel->NextSibling))
                {
                    next->PrevSibling = rel->PrevSibling;
                }
            }
        }
        std::vector<std::pair<std::shared_ptr<ScriptableComponent>, bool>> instances;
        if (auto* storage = m_EntityRegistry.try_get<ScriptStorage>(entity))
        {
            for (auto& script : storage->Scripts)
            {
                if (script.Instance && script.Awakened)
                {
                    instances.emplace_back(script.Instance, script.Active);
                }
            }
            m_EntityRegistry.remove<ScriptStorage>(entity);
        }
        for (const auto& [instance, active] : instances)
        {
            if (active)
            {
                instance->OnDisable();
            }
            instance->OnDestroy();
        }
        if (!m_EntityRegistry.valid(entity))
        {
            return;
        }
        m_EntityRegistry.destroy(entity);
    }

    Entity Scene::DuplicateEntity(entt::entity handle)
    {
        if (!m_EntityRegistry.valid(handle))
        {
            return {};
        }
        const auto parent = m_EntityRegistry.get<RelationshipComponent>(handle).Parent;
        auto copy = InstantiateHierarchy(*this, handle);
        copy.GetComponent<TagComponent>().tag += " Copy";
        if (parent != entt::null)
        {
            copy.SetParent(GetEntity(parent));
        }
        return copy;
    }

    Entity Scene::InstantiateHierarchy(const Scene& source, entt::entity handle)
    {
        std::unordered_map<uint64_t, uint64_t> remap;
        std::vector<entt::entity> copies;
        std::function<Entity(entt::entity)> clone = [&](entt::entity handle) -> Entity
        {
            if (!source.m_EntityRegistry.valid(handle))
            {
                return {};
            }
            auto copy = CreateEntity(source.m_EntityRegistry.get<TagComponent>(handle).tag);
            const auto target = copy.GetHandle();
            remap[source.m_EntityRegistry.get<TagComponent>(handle).uuid] = copy.GetComponent<TagComponent>().uuid;
            copies.push_back(target);
            if (const auto* group = source.m_EntityRegistry.try_get<LODGroupComponent>(handle))
            {
                m_EntityRegistry.emplace<LODGroupComponent>(target, *group);
            }
            if (const auto* transform = source.m_EntityRegistry.try_get<TransformComponent>(handle))
            {
                m_EntityRegistry.replace<TransformComponent>(target, *transform);
            }
            if (const auto* mesh = source.m_EntityRegistry.try_get<MeshComponent>(handle))
            {
                const auto saved = *mesh;
                m_EntityRegistry.emplace<MeshComponent>(target, saved);
            }
            if (const auto* material = source.m_EntityRegistry.try_get<MaterialComponent>(handle))
            {
                auto component = *material;
                if (component.MaterialInstance)
                {
                    component.MaterialInstance = component.MaterialInstance->Clone();
                }
                m_EntityRegistry.emplace<MaterialComponent>(target, std::move(component));
            }
            if (const auto* camera = source.m_EntityRegistry.try_get<CameraComponent>(handle))
            {
                m_EntityRegistry.emplace<CameraComponent>(target, *camera).IsPrimary = false;
            }
            if (const auto* storage = source.m_EntityRegistry.try_get<ScriptStorage>(handle))
            {
                struct Copy
                {
                    std::string Type;
                    bool Enabled;
                    std::map<std::string, ScriptValue> Properties;
                };
                std::vector<Copy> scripts;
                for (const auto& script : storage->Scripts)
                {
                    if (script.TypeName.empty())
                    {
                        continue;
                    }
                    Copy data{ script.TypeName, script.Enabled, script.Properties };
                    if (const auto* definition = ScriptRegistry::Find(script.TypeName); definition && script.Instance)
                    {
                        for (const auto& property : definition->Properties)
                        {
                            data.Properties[property.Name] = property.Get(*script.Instance);
                        }
                    }
                    scripts.push_back(std::move(data));
                }
                for (const auto& data : scripts)
                {
                    ScriptRegistry::Attach(*this, target, data.Type);
                    auto& script = m_EntityRegistry.get<ScriptStorage>(target).Scripts.back();
                    script.Enabled = data.Enabled;
                    script.Properties = data.Properties;
                    if (const auto* definition = ScriptRegistry::Find(data.Type); definition && script.Instance)
                    {
                        for (const auto& property : definition->Properties)
                        {
                            if (const auto found = data.Properties.find(property.Name); found != data.Properties.end())
                            {
                                property.Set(*script.Instance, found->second);
                            }
                        }
                    }
                }
            }
            const auto relationship = source.m_EntityRegistry.get<RelationshipComponent>(handle);

            auto child = relationship.FirstChild;
            while (child != entt::null && source.m_EntityRegistry.valid(child))
            {
                const auto next = source.m_EntityRegistry.get<RelationshipComponent>(child).NextSibling;
                auto duplicate = clone(child);
                duplicate.SetParent(copy);
                child = next;
            }
            return copy;
        };
        auto result = clone(handle);
        for (auto target : copies)
        {
            if (auto* group = m_EntityRegistry.try_get<LODGroupComponent>(target))
            {
                for (auto& level : group->Levels)
                {
                    for (auto& id : level.Renderers)
                    {
                        if (const auto found = remap.find(id); found != remap.end())
                        {
                            id = found->second;
                        }
                        else if (&source != this)
                        {
                            id = 0;
                        }
                    }
                    std::erase(level.Renderers, uint64_t(0));
                }
            }
        }
        return result;
    }

    void Scene::CopyScriptsTo(Scene& destination) const
    {
        std::unordered_map<uint64_t, entt::entity> targets;
        for (auto target : destination.m_EntityRegistry.view<TagComponent>())
        {
            targets[static_cast<uint64_t>(destination.m_EntityRegistry.get<TagComponent>(target).uuid)] = target;
        }

        for (auto source : m_EntityRegistry.view<TagComponent>())
        {
            const auto uuid = static_cast<uint64_t>(m_EntityRegistry.get<TagComponent>(source).uuid);
            const auto found = targets.find(uuid);
            if (found == targets.end())
            {
                continue;
            }
            const auto target = found->second;
            if (const auto* mesh = m_EntityRegistry.try_get<MeshComponent>(source))
            {
                if (auto* existing = destination.m_EntityRegistry.try_get<MeshComponent>(target))
                {
                    if (!existing->MeshData)
                    {
                        existing->MeshData = mesh->MeshData;
                    }
                }
                else
                {
                    destination.m_EntityRegistry.emplace<MeshComponent>(target, *mesh);
                }
            }
            if (const auto* material = m_EntityRegistry.try_get<MaterialComponent>(source))
            {
                if (auto* existing = destination.m_EntityRegistry.try_get<MaterialComponent>(target))
                {
                    if (!existing->MaterialInstance && material->MaterialInstance)
                    {
                        existing->MaterialInstance = material->MaterialInstance->Clone();
                    }
                }
                else
                {
                    destination.m_EntityRegistry.emplace<MaterialComponent>(target, *material);
                }
            }
        }

        for (auto source : m_EntityRegistry.view<ScriptStorage, TagComponent>())
        {
            const auto uuid = static_cast<uint64_t>(m_EntityRegistry.get<TagComponent>(source).uuid);
            const auto found = targets.find(uuid);
            if (found == targets.end())
            {
                continue;
            }
            auto& destinationStorage = destination.m_EntityRegistry.get_or_emplace<ScriptStorage>(found->second);
            const auto& storage = m_EntityRegistry.get<ScriptStorage>(source);
            for (const auto& item : storage.Scripts)
            {
                if (!item.TypeName.empty())
                {
                    continue;
                }
                if (!item.Factory)
                {
                    continue;
                }
                auto script = item.Factory();
                if (!script)
                {
                    continue;
                }
                script->m_EntityHandle = found->second;
                script->m_CurrentScene = &destination;
                script->m_Lifetime = destination.GetLifetime();
                destinationStorage.Scripts.emplace_back(std::move(script), false, item.Factory);
            }
        }
    }
} // namespace FINALITY

FINALITY::Entity FINALITY::Scene::GetEntity(entt::entity handle)
{
    if (!m_EntityRegistry.valid(handle))
    {
        return Entity{};
    }
    return Entity(handle, this);
}
