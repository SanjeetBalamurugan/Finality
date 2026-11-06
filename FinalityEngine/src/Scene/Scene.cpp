#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Renderer/Renderer.h"
#include <unordered_map>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

namespace FINALITY
{
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
            auto& storage = m_EntityRegistry.get<ScriptStorage>(entityHandle);
            for (size_t i = 0; i < storage.Scripts.size(); ++i)
            {
                auto& script = storage.Scripts[i];

                if (!script.Instance)
                    continue;

                if (!script.Started)
                {
                    script.Instance->Start();
                    script.Started = true;
                }

                script.Instance->Update(ts);
            }
        }
    }

    void Scene::OnRender()
    {
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

            if (cam.IsPrimary)
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
        auto view = m_EntityRegistry.view<ScriptStorage>();

        for (auto entityHandle : view)
        {
            auto& storage = view.get<ScriptStorage>(entityHandle);

            for (auto& script : storage.Scripts)
            {
                if (script.Instance)
                    script.Instance->OnDestroy();
            }
        }

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
        if (auto* rel = m_EntityRegistry.try_get<RelationshipComponent>(entity))
        {
            auto child = rel->FirstChild;
            while (child != entt::null && m_EntityRegistry.valid(child))
            {
                const auto* nextRel = m_EntityRegistry.try_get<RelationshipComponent>(child);
                const auto next = nextRel ? nextRel->NextSibling : entt::null;
                DestroyEntity(child);
                child = next;
            }
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
        if (auto* storage = m_EntityRegistry.try_get<ScriptStorage>(entity))
            for (auto& script : storage->Scripts)
                if (script.Instance) script.Instance->OnDestroy();
        m_EntityRegistry.destroy(entity);
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
                if (!item.Factory) continue;
                auto script = item.Factory();
                if (!script) continue;
                script->m_EntityHandle = found->second;
                script->m_CurrentScene = &destination;
                script->Awake();
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
