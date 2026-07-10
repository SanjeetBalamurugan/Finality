#pragma once

#include <memory>
#include <vector>

#include <Core/Core.h>

namespace FINALITY
{
    class Entity;
    class Scene;

    class ScriptableComponent;

    struct ScriptStorage
    {
        struct ScriptInstance
        {
            std::unique_ptr<ScriptableComponent> Instance;
            bool Started = false;

            ScriptInstance(std::unique_ptr<ScriptableComponent> instance, bool started)
                : Instance(std::move(instance)), Started(started)
            {
            }

            ScriptInstance(ScriptInstance&&) noexcept = default;
            ScriptInstance& operator=(ScriptInstance&&) noexcept = default;

            ScriptInstance(const ScriptInstance&) = delete;
            ScriptInstance& operator=(const ScriptInstance&) = delete;
        };

        std::vector<ScriptInstance> Scripts;

        ScriptStorage() = default;

        ScriptStorage(ScriptStorage&&) noexcept = default;
        ScriptStorage& operator=(ScriptStorage&&) noexcept = default;

        ScriptStorage(const ScriptStorage&) = delete;
        ScriptStorage& operator=(const ScriptStorage&) = delete;
    };

    class FAPI ScriptableComponent
    {
    public:
        virtual ~ScriptableComponent() = default;

        Entity& GetEntity();
        const Entity& GetEntity() const;


    protected:
        template<typename T>
        T& GetComponent();

        template<typename T>
        bool HasComponent() const;


    private:
        virtual void Awake() {}
        virtual void Start() {}
        virtual void Update(float dt) {}
        virtual void OnDestroy() {}

        Entity* m_Entity = nullptr;

        friend class Scene;
        friend class Entity;
    };
}

#include "ScriptableComponent.inl"