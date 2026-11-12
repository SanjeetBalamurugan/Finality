#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Renderer/Renderer.h"
#include <unordered_map>
#include <glm/gtc/quaternion.hpp>
#include <Scene/ScriptRegistry.h>

namespace FINALITY
{
    Scene::Scene() = default;
    Scene::~Scene() = default;
    glm::mat4 Scene::GetWorldTransform(entt::entity entityHandle) const
    {
        auto* transform = m_EntityRegistry.try_get<TransformComponent>(entityHandle);
        if (!transform) return glm::mat4(1.0f);

        glm::mat4 worldMatrix = transform->GetTransformMatrix();

        auto* rel = m_EntityRegistry.try_get<RelationshipComponent>(entityHandle);
        entt::entity parentHandle = rel ? rel->Parent : entt::null;

        size_t hops = 0;
        while (parentHandle != entt::null && m_EntityRegistry.valid(parentHandle)
            && hops++ < 1024)
        {
            auto* parentTransform = m_EntityRegistry.try_get<TransformComponent>(parentHandle);
            if (!parentTransform) break;

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

    void Scene::OnUpdate(float ts)
    {
        auto scriptView = m_EntityRegistry.view<ScriptStorage>();

        std::vector entities(scriptView.begin(), scriptView.end());
        for (entt::entity entityHandle : entities)
        {
            if (!m_EntityRegistry.valid(entityHandle)) continue;
            auto* storage = m_EntityRegistry.try_get<ScriptStorage>(entityHandle);
            if (!storage) continue;
            std::vector<std::shared_ptr<ScriptableComponent>> instances;
            for (const auto& script : storage->Scripts)
                if (script.Enabled && script.Instance) instances.push_back(script.Instance);
            for (const auto& instance : instances)
            {
                auto find = [&]() -> ScriptStorage::ScriptInstance*
                {
                    if (!m_EntityRegistry.valid(entityHandle)) return nullptr;
                    auto* current = m_EntityRegistry.try_get<ScriptStorage>(entityHandle);
                    if (!current) return nullptr;
                    for (auto& entry : current->Scripts)
                        if (entry.Instance == instance && entry.Enabled) return &entry;
                    return nullptr;
                };
                auto* script = find();
                if (!script) continue;
                if (!script->Awakened)
                {
                    script->Awakened = true;
                    instance->Awake();
                }
                script = find();
                if (!script) continue;
                if (!script->Started)
                {
                    script->Started = true;
                    instance->Start();
                }
                if (find()) instance->Update(ts);
            }
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

        auto cameraView = m_EntityRegistry.view<CameraComponent, TransformComponent>();
        for (auto entityHandle : cameraView)
        {
            auto& cam = cameraView.get<CameraComponent>(entityHandle);
            auto& transform = cameraView.get<TransformComponent>(entityHandle);
            (void)transform;

            if (cam.IsPrimary && cam.CameraInstance)
            {
                const glm::mat4 world = GetWorldTransform(entityHandle);
                cam.CameraInstance->SetPosition(glm::vec3(world[3]));
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
            glm::mat4 worldTransform = GetWorldTransform(entityHandle);
            Renderer::PushEntity(entity, worldTransform);
        }

        Renderer::EndScene();
    }

    void Scene::OnDestroy()
    {
        std::vector<std::shared_ptr<ScriptableComponent>> instances;
        for (auto entityHandle : m_EntityRegistry.view<ScriptStorage>())
            for (auto& script : m_EntityRegistry.get<ScriptStorage>(entityHandle).Scripts)
                if (script.Instance && script.Awakened)
                {
                    script.Awakened = false;
                    instances.push_back(script.Instance);
                }
        for (const auto& instance : instances) instance->OnDestroy();

        if (Renderer::GetDevice())
            Renderer::GetDevice()->WaitIdle();

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
        if (!m_EntityRegistry.valid(entity)) return;
        if (Renderer::GetDevice()) Renderer::GetDevice()->WaitIdle();
        if (auto* rel = m_EntityRegistry.try_get<RelationshipComponent>(entity))
        {
            const auto relationship = *rel;
            auto child = relationship.FirstChild;
            while (child != entt::null && m_EntityRegistry.valid(child))
            {
                const auto* nextRel = m_EntityRegistry.try_get<RelationshipComponent>(child);
                const auto next = nextRel ? nextRel->NextSibling : entt::null;
                DestroyEntity(child);
                child = next;
            }
            rel = m_EntityRegistry.try_get<RelationshipComponent>(entity);
            if (!rel) return;
            if (rel->Parent != entt::null && m_EntityRegistry.valid(rel->Parent))
            {
                if (auto* parent = m_EntityRegistry.try_get<RelationshipComponent>(rel->Parent))
                    if (parent->FirstChild == entity) parent->FirstChild = rel->NextSibling;
            }
            if (rel->PrevSibling != entt::null && m_EntityRegistry.valid(rel->PrevSibling))
                if (auto* previous = m_EntityRegistry.try_get<RelationshipComponent>(rel->PrevSibling))
                    previous->NextSibling = rel->NextSibling;
            if (rel->NextSibling != entt::null && m_EntityRegistry.valid(rel->NextSibling))
                if (auto* next = m_EntityRegistry.try_get<RelationshipComponent>(rel->NextSibling))
                    next->PrevSibling = rel->PrevSibling;
        }
        std::vector<std::shared_ptr<ScriptableComponent>> instances;
        if (auto* storage = m_EntityRegistry.try_get<ScriptStorage>(entity))
        {
            for (auto& script : storage->Scripts)
                if (script.Instance && script.Awakened) instances.push_back(script.Instance);
            m_EntityRegistry.remove<ScriptStorage>(entity);
        }
        for (const auto& instance : instances) instance->OnDestroy();
        if (!m_EntityRegistry.valid(entity)) return;
        m_EntityRegistry.destroy(entity);
    }

    Entity Scene::DuplicateEntity(entt::entity handle)
    {
        if (!m_EntityRegistry.valid(handle)) return {};
        auto copy = CreateEntity(m_EntityRegistry.get<TagComponent>(handle).tag + " Copy");
        const auto target = copy.GetHandle();
        if (const auto* transform = m_EntityRegistry.try_get<TransformComponent>(handle)) m_EntityRegistry.replace<TransformComponent>(target, *transform);
        if (const auto* mesh = m_EntityRegistry.try_get<MeshComponent>(handle)) m_EntityRegistry.emplace<MeshComponent>(target, *mesh);
        if (const auto* material = m_EntityRegistry.try_get<MaterialComponent>(handle)) m_EntityRegistry.emplace<MaterialComponent>(target, *material);
        if (const auto* camera = m_EntityRegistry.try_get<CameraComponent>(handle)) m_EntityRegistry.emplace<CameraComponent>(target, *camera).IsPrimary = false;
        if (const auto* storage = m_EntityRegistry.try_get<ScriptStorage>(handle))
        {
            struct Copy { std::string Type; bool Enabled; std::map<std::string, ScriptValue> Properties; };
            std::vector<Copy> scripts;
            for (const auto& script : storage->Scripts)
            {
                if (script.TypeName.empty()) continue;
                Copy data{script.TypeName, script.Enabled, script.Properties};
                if (const auto* definition = ScriptRegistry::Find(script.TypeName); definition && script.Instance)
                    for (const auto& property : definition->Properties) data.Properties[property.Name] = property.Get(*script.Instance);
                scripts.push_back(std::move(data));
            }
            for (const auto& data : scripts)
            {
                ScriptRegistry::Attach(*this, target, data.Type);
                auto& script = m_EntityRegistry.get<ScriptStorage>(target).Scripts.back();
                script.Enabled = data.Enabled;
                script.Properties = data.Properties;
                if (const auto* definition = ScriptRegistry::Find(data.Type); definition && script.Instance)
                    for (const auto& property : definition->Properties)
                        if (const auto found = data.Properties.find(property.Name); found != data.Properties.end()) property.Set(*script.Instance, found->second);
            }
        }
        const auto relationship = m_EntityRegistry.get<RelationshipComponent>(handle);
        if (relationship.Parent != entt::null) copy.SetParent(GetEntity(relationship.Parent));
        auto child = relationship.FirstChild;
        while (child != entt::null && m_EntityRegistry.valid(child))
        {
            const auto next = m_EntityRegistry.get<RelationshipComponent>(child).NextSibling;
            auto duplicate = DuplicateEntity(child);
            duplicate.SetParent(copy);
            child = next;
        }
        return copy;
    }

    Entity Scene::InstantiateHierarchy(const Scene& source, entt::entity handle)
    {
        if (!source.m_EntityRegistry.valid(handle)) return {};
        auto copy = CreateEntity(source.m_EntityRegistry.get<TagComponent>(handle).tag);
        const auto target = copy.GetHandle();
        if (const auto* transform = source.m_EntityRegistry.try_get<TransformComponent>(handle)) m_EntityRegistry.replace<TransformComponent>(target, *transform);
        if (const auto* mesh = source.m_EntityRegistry.try_get<MeshComponent>(handle)) m_EntityRegistry.emplace<MeshComponent>(target, *mesh);
        if (const auto* material = source.m_EntityRegistry.try_get<MaterialComponent>(handle))
        {
            auto component = *material;
            if (component.MaterialInstance) component.MaterialInstance = component.MaterialInstance->Clone();
            m_EntityRegistry.emplace<MaterialComponent>(target, std::move(component));
        }
        if (const auto* camera = source.m_EntityRegistry.try_get<CameraComponent>(handle)) m_EntityRegistry.emplace<CameraComponent>(target, *camera).IsPrimary = false;
        if (const auto* storage = source.m_EntityRegistry.try_get<ScriptStorage>(handle))
        {
            struct Copy { std::string Type; bool Enabled; std::map<std::string, ScriptValue> Properties; };
            std::vector<Copy> scripts;
            for (const auto& script : storage->Scripts)
            {
                if (script.TypeName.empty()) continue;
                Copy data{script.TypeName, script.Enabled, script.Properties};
                if (const auto* definition = ScriptRegistry::Find(script.TypeName); definition && script.Instance)
                    for (const auto& property : definition->Properties) data.Properties[property.Name] = property.Get(*script.Instance);
                scripts.push_back(std::move(data));
            }
            for (const auto& data : scripts)
            {
                ScriptRegistry::Attach(*this, target, data.Type);
                auto& script = m_EntityRegistry.get<ScriptStorage>(target).Scripts.back();
                script.Enabled = data.Enabled;
                script.Properties = data.Properties;
                if (const auto* definition = ScriptRegistry::Find(data.Type); definition && script.Instance)
                    for (const auto& property : definition->Properties)
                        if (const auto found = data.Properties.find(property.Name); found != data.Properties.end()) property.Set(*script.Instance, found->second);
            }
        }
        const auto relationship = source.m_EntityRegistry.get<RelationshipComponent>(handle);

        auto child = relationship.FirstChild;
        while (child != entt::null && source.m_EntityRegistry.valid(child))
        {
            const auto next = source.m_EntityRegistry.get<RelationshipComponent>(child).NextSibling;
            auto duplicate = InstantiateHierarchy(source, child);
            duplicate.SetParent(copy);
            child = next;
        }
        return copy;
    }

    void Scene::CopyScriptsTo(Scene& destination) const
    {
        std::unordered_map<uint64_t, entt::entity> targets;
        for (auto target : destination.m_EntityRegistry.view<TagComponent>())
            targets[static_cast<uint64_t>(destination.m_EntityRegistry.get<TagComponent>(target).uuid)] = target;

        for (auto source : m_EntityRegistry.view<TagComponent>())
        {
            const auto uuid = static_cast<uint64_t>(m_EntityRegistry.get<TagComponent>(source).uuid);
            const auto found = targets.find(uuid);
            if (found == targets.end()) continue;
            const auto target = found->second;
            if (const auto* mesh = m_EntityRegistry.try_get<MeshComponent>(source))
            {
                if (auto* existing = destination.m_EntityRegistry.try_get<MeshComponent>(target))
                {
                    if (!existing->MeshData) existing->MeshData = mesh->MeshData;
                }
                else destination.m_EntityRegistry.emplace<MeshComponent>(target, *mesh);
            }
            if (const auto* material = m_EntityRegistry.try_get<MaterialComponent>(source))
            {
                if (auto* existing = destination.m_EntityRegistry.try_get<MaterialComponent>(target))
                {
                    if (!existing->MaterialInstance && material->MaterialInstance)
                        existing->MaterialInstance = material->MaterialInstance->Clone();
                }
                else destination.m_EntityRegistry.emplace<MaterialComponent>(target, *material);
            }
        }

        for (auto source : m_EntityRegistry.view<ScriptStorage, TagComponent>())
        {
            const auto uuid = static_cast<uint64_t>(m_EntityRegistry.get<TagComponent>(source).uuid);
            const auto found = targets.find(uuid);
            if (found == targets.end()) continue;
            auto& destinationStorage = destination.m_EntityRegistry.get_or_emplace<ScriptStorage>(found->second);
            const auto& storage = m_EntityRegistry.get<ScriptStorage>(source);
            for (const auto& item : storage.Scripts)
            {
                if (!item.TypeName.empty()) continue;
                if (!item.Factory) continue;
                auto script = item.Factory();
                if (!script) continue;
                script->m_EntityHandle = found->second;
                script->m_CurrentScene = &destination;
                destinationStorage.Scripts.emplace_back(std::move(script), false, item.Factory);
            }
        }
    }
}
FINALITY::Entity FINALITY::Scene::GetEntity(entt::entity handle)
{
    if (!m_EntityRegistry.valid(handle)) return Entity{};
    return Entity(handle, this);
}
