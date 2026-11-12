#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <entt.hpp>
#include <Core/Core.h>
#include <string>
#include <map>
#include <variant>
#include <glm/glm.hpp>
#include <Assets/AssetRegistry.h>

#ifdef FINALITY_REFLECTION_SCAN
#define FISerializeField friend struct ::FINALITY::ScriptFieldAccess; [[clang::annotate("finality.serialize")]]
#else
#define FISerializeField friend struct ::FINALITY::ScriptFieldAccess;
#endif

namespace FINALITY
{
    struct ScriptFieldAccess;
    class Entity;
    class Scene;

    class ScriptableComponent;

    struct ScriptAssetReference
    {
        ProjectAssetType Type = ProjectAssetType::Unknown;
        std::string Reference;
    };

    using ScriptValue = std::variant<bool, int, float, std::string, glm::vec3, ScriptAssetReference>;

    struct ScriptStorage
    {
        struct ScriptInstance
        {
            std::shared_ptr<void> ModuleLease;
            std::shared_ptr<ScriptableComponent> Instance;
            bool Started = false;
            bool Awakened = false;
            bool Enabled = true;
            std::string TypeName;
            std::map<std::string, ScriptValue> Properties;
            std::function<std::unique_ptr<ScriptableComponent>()> Factory;

            FAPI ScriptInstance(std::unique_ptr<ScriptableComponent> instance, bool started,
                std::function<std::unique_ptr<ScriptableComponent>()> factory = {}, std::shared_ptr<void> moduleLease = {});

            ScriptInstance(ScriptInstance&&) noexcept = default;
            FAPI ScriptInstance& operator=(ScriptInstance&& other) noexcept;

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

        Entity GetEntity();
        const Entity GetEntity() const;

    protected:
        template<typename T>
        bool HasComponent() const;

        template<typename T>
        T& GetComponent();

    protected:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_CurrentScene = nullptr;

    private:
        virtual void Awake() {}
        virtual void Start() {}
        virtual void Update(float dt) {}
        virtual void OnDestroy() {}

        friend class Scene;
        friend class Entity;
        friend class ScriptRegistry;
    };
}

#include "ScriptableComponent.inl"
