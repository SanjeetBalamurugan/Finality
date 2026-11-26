/**
 * @file
 * @ingroup scene
 * @brief Defines native script ownership, lifecycle and Inspector field annotations.
 *
 * Scene binds each attached script to its entity and controls its callbacks. Public mutable supported fields are exposed automatically. FISerializeField opts private fields into generated access; it does not convert arbitrary unsupported C++ types into Inspector values.
 *
 * @par Usage
 * Override the needed lifecycle callbacks. Initial activation runs Awake, OnEnable and Start. Re-enabling repeats OnEnable; active destruction dispatches OnDisable before OnDestroy. Use GetEntity for scene operations.
 */
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
#define FISerializeField                         \
    friend struct ::FINALITY::ScriptFieldAccess; \
    [[clang::annotate("finality.serialize")]]
#else
#define FISerializeField friend struct ::FINALITY::ScriptFieldAccess;
#endif

namespace FINALITY
{
    struct ScriptFieldAccess;
    class Entity;
    class Scene;

    /**
     * @ingroup scene
     * @brief Defines native script ownership, lifecycle and Inspector field annotations.
     *
     * Scene binds each attached script to its entity and controls its callbacks. Public mutable supported fields are exposed automatically. FISerializeField opts private fields into generated access; it does not convert arbitrary unsupported C++ types into Inspector values.
     *
     * @par Usage
     * Override the needed lifecycle callbacks. Initial activation runs Awake, OnEnable and Start. Re-enabling repeats OnEnable; active destruction dispatches OnDisable before OnDestroy. Use GetEntity for scene operations.
     */
    class ScriptableComponent;

    /** @brief Typed Inspector asset reference stored as a registry reference or path. */
    struct ScriptAssetReference
    {
        ProjectAssetType Type = ProjectAssetType::Unknown;
        std::string Reference;
    };

    /** @todo TODO: Document ScriptValue alias semantics, ownership and usage. */
    using ScriptValue = std::variant<bool, int, float, std::string, glm::vec3, ScriptAssetReference>;

    /** @brief Attached native script instances and their authored property values for one entity. */
    struct ScriptStorage
    {
        /** @brief Script instance state and its module lease; keeps native code alive while the instance exists. */
        struct ScriptInstance
        {
            std::shared_ptr<void> ModuleLease;
            std::shared_ptr<ScriptableComponent> Instance;
            bool Started = false;
            bool Awakened = false;
            bool Active = false;
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

    /** @todo TODO: Document ScriptableComponent class semantics, ownership and usage. */
    class FAPI ScriptableComponent
    {
    public:
        virtual ~ScriptableComponent() = default;

        Entity GetEntity();
        const Entity GetEntity() const;

    protected:
        template <typename T>
        bool HasComponent() const;

        template <typename T>
        T& GetComponent();

    protected:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_CurrentScene = nullptr;
        std::weak_ptr<void> m_Lifetime;

    private:
        virtual void Awake()
        {
        }

        virtual void Start()
        {
        }

        virtual void OnEnable()
        {
        }

        virtual void OnDisable()
        {
        }

        virtual void FixedUpdate(float dt)
        {
        }

        virtual void Update(float dt)
        {
        }

        virtual void LateUpdate(float dt)
        {
        }

        virtual void OnDestroy()
        {
        }

        friend class Scene;
        friend class Entity;
        friend class ScriptRegistry;
    };
} // namespace FINALITY

#include "ScriptableComponent.inl"
