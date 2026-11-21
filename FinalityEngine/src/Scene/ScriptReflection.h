/**
 * @file
 * @ingroup scene
 * @brief Builds typed field metadata for native script registration.
 *
 * Generated wrappers specialize ScriptFieldAccess for discovered script and nested struct types. Templates flatten the supported fields into the ScriptDefinition property descriptors consumed by serialization and the Inspector.
 *
 * @par Usage
 * Run FIScriptReflect before compiling script translation units. Compile the generated wrappers instead of compiling the same authored cpp files a second time.
 */
#pragma once

#include "ScriptRegistry.h"
#include <tuple>
#include <typeindex>
#include <set>
#include <algorithm>

namespace FINALITY
{
    /** @todo TODO: Document ScriptField struct semantics, ownership and usage. */
    template<typename T, typename V>
    struct ScriptField
    {
        const char* Name;
        V T::* Member;
    };

    template<typename T, typename V>
    auto MakeScriptField(const char* name, V T::* member)
    {
        return ScriptField<T, V>{name, member};
    }

    /** @todo TODO: Document ScriptFieldAccess struct semantics, ownership and usage. */
    struct ScriptFieldAccess
    {
        template<typename T>
        static auto Members() { return std::tuple{}; }
    };

    /** @todo TODO: Document ScriptReflection class semantics, ownership and usage. */
    class ScriptReflection
    {
        template<typename V>
        static constexpr bool IsValue = std::is_same_v<V, bool> || std::is_same_v<V, int> || std::is_same_v<V, float> ||
            std::is_same_v<V, std::string> || std::is_same_v<V, glm::vec3> || std::is_same_v<V, ScriptAssetReference>;
        /** @todo TODO: Document Entry struct semantics, ownership and usage. */
        struct Entry
        {
            std::type_index Type;
            ScriptDefinition Definition;
        };

        static auto& Entries()
        {
            static std::vector<Entry> entries;
            return entries;
        }

        template<typename Root, typename Access>
        static void Append(std::vector<ScriptProperty>& properties, const std::string& name, Access access)
        {
            /** @todo TODO: Document V alias semantics, ownership and usage. */
            using V = std::remove_cvref_t<decltype(access(std::declval<Root&>()))>;
            if constexpr (IsValue<V>)
            {
                properties.push_back({name,
                    [access](ScriptableComponent& script) -> ScriptValue { return access(static_cast<Root&>(script)); },
                    [access](ScriptableComponent& script, const ScriptValue& value)
                    {
                        auto& field = access(static_cast<Root&>(script));
                        if constexpr (std::is_same_v<V, ScriptAssetReference>)
                            if (field.Type != std::get<V>(value).Type) return;
                        field = std::get<V>(value);
                    }});
            }
            else
            {
                std::apply([&](auto... fields)
                {
                    (Append<Root>(properties, name.empty() ? fields.Name : name + "." + fields.Name,
                        [access, member = fields.Member](Root& script) -> auto& { return access(script).*member; }), ...);
                }, ScriptFieldAccess::Members<V>());
            }
        }

    public:
        template<typename T>
        static bool Register(const char* name, const char* source = "")
        {
            const auto type = std::type_index(typeid(T));
            for (const auto& entry : Entries()) if (entry.Type == type) return true;
            ScriptDefinition definition{name, []() { return std::make_unique<T>(); }};
            definition.SourceFile = source;
            std::apply([&](auto... fields)
            {
                ([&]
                {
                    /** @todo TODO: Document V alias semantics, ownership and usage. */
                    using V = std::remove_cvref_t<decltype(std::declval<T&>().*fields.Member)>;
                    if constexpr (IsValue<V>) definition.Properties.push_back(ScriptRegistry::Property(fields.Name, fields.Member));
                    else Append<T>(definition.Properties, fields.Name,
                        [member = fields.Member](T& script) -> auto& { return script.*member; });
                }(), ...);
            }, ScriptFieldAccess::Members<T>());
            Entries().push_back({type, std::move(definition)});
            return true;
        }

        template<typename AddLegacy>
        static bool RegisterModule(AddLegacy legacy, const std::function<bool(const ScriptDefinition&)>& add)
        {
            std::set<std::type_index> registered;
            const auto merge = [&](const ScriptDefinition& original)
            {
                auto definition = original;
                const auto instance = definition.Factory ? definition.Factory() : nullptr;
                if (!instance) return false;
                const auto type = std::type_index(typeid(*instance));
                for (const auto& entry : Entries())
                    if (entry.Type == type)
                    {
                        definition.SourceFile = entry.Definition.SourceFile;
                        for (const auto& property : entry.Definition.Properties)
                        {
                            const auto existing = std::find_if(definition.Properties.begin(), definition.Properties.end(),
                                [&](const auto& field) { return field.Name == property.Name ||
                                    (property.MatchesMember && property.MatchesMember(field.Member)); });
                            if (existing == definition.Properties.end()) definition.Properties.push_back(property);
                        }
                        break;
                    }
                registered.insert(type);
                return add(definition);
            };
            if (!legacy(merge)) return false;
            for (const auto& entry : Entries())
                if (!registered.contains(entry.Type) && !add(entry.Definition)) return false;
            return true;
        }
    };
}
