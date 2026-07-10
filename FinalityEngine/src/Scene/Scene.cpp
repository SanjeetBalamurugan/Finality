#include "Scene.h"
#include "Entity.h"

#include "Components.h"

namespace FINALITY
{
    void Scene::OnUpdate(float ts)
    {
        auto view = m_EntityRegistry.view<ScriptStorage>();

        for (auto entityHandle : view)
        {
            auto& storage = view.get<ScriptStorage>(entityHandle);

            for (auto& script : storage.Scripts)
            {
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