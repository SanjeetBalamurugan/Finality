#pragma once

#include "ScriptableComponent.h"
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <variant>
#include <any>

namespace FINALITY
{
    struct ScriptProperty
    {
        std::string Name;
        std::function<ScriptValue(ScriptableComponent&)> Get;
        std::function<void(ScriptableComponent&, const ScriptValue&)> Set;
        std::any Member;
        std::function<bool(const std::any&)> MatchesMember;
    };

    struct ScriptDefinition
    {
        std::shared_ptr<void> ModuleLease;
        std::string Name;
        std::function<std::unique_ptr<ScriptableComponent>()> Factory;
        std::vector<ScriptProperty> Properties;
        std::string SourceFile;
        ScriptDefinition() = default;
        ScriptDefinition(std::string name, std::function<std::unique_ptr<ScriptableComponent>()> factory, std::vector<ScriptProperty> properties = {})
            : Name(std::move(name)), Factory(std::move(factory)), Properties(std::move(properties)) {}
    };

    class FAPI ScriptRegistry
    {
    public:
        static bool Register(const ScriptDefinition& definition);
        static const std::map<std::string, ScriptDefinition>& GetDefinitions();
        static const ScriptDefinition* Find(const std::string& name);
        static bool Attach(Scene& scene, entt::entity entity, const std::string& name);
        static void Remove(Scene& scene, entt::entity entity, size_t index);
        static bool ReplaceModule(const std::vector<ScriptDefinition>& definitions, const std::shared_ptr<void>& lease, Scene* scene, std::string& error);
        static void ClearModule();

        template<typename T, typename V>
        static ScriptProperty Property(const std::string& name, V T::* member)
        {
            return {name,
                [member](ScriptableComponent& script) -> ScriptValue { return static_cast<T&>(script).*member; },
                [member](ScriptableComponent& script, const ScriptValue& value)
                {
                    if constexpr (std::is_same_v<V, ScriptAssetReference>)
                        if ((static_cast<T&>(script).*member).Type != std::get<V>(value).Type) return;
                    static_cast<T&>(script).*member = std::get<V>(value);
                }, member,
                [member](const std::any& candidate)
                {
                    const auto* other = std::any_cast<V T::*>(&candidate);
                    return other && *other == member;
                }};
        }
    };
}
