#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Renderer/Renderer.h"

namespace FINALITY
{
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

        auto cameraView = m_EntityRegistry.view<CameraComponent, TransformComponent>();
        for (auto entityHandle : cameraView)
        {
            auto& cam = cameraView.get<CameraComponent>(entityHandle);
            auto& transform = cameraView.get<TransformComponent>(entityHandle);

            if (cam.IsPrimary)
            {
                Renderer::SetActiveCamera(cam.CameraInstance.get());
            }
        }

        Renderer::BeginScene();

        auto renderView = m_EntityRegistry.view<MeshComponent>();
        for (auto entityHandle : renderView)
        {
            Entity entity{ entityHandle, this };
            Renderer::PushEntity(entity);
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

        return entity;
    }
}
