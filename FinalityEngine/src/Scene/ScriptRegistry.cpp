#include "ScriptRegistry.h"
#include "Scene.h"
#include <Core/Log.h>
#include <set>

namespace
{
    std::map<std::string, FINALITY::ScriptDefinition> s_Definitions;
    std::set<std::string> s_ModuleNames;
}

bool FINALITY::ScriptRegistry::Register(const ScriptDefinition& definition)
{
    if (definition.Name.empty() || !definition.Factory || s_Definitions.contains(definition.Name))
        return false;
    s_Definitions.emplace(definition.Name, definition);
    return true;
}

const std::map<std::string, FINALITY::ScriptDefinition>& FINALITY::ScriptRegistry::GetDefinitions()
{
    return s_Definitions;
}

const FINALITY::ScriptDefinition* FINALITY::ScriptRegistry::Find(const std::string& name)
{
    const auto found = s_Definitions.find(name);
    return found == s_Definitions.end() ? nullptr : &found->second;
}

bool FINALITY::ScriptRegistry::Attach(Scene& scene, entt::entity entity, const std::string& name)
{
    auto& registry = scene.GetRegistry();
    if (!registry.valid(entity)) return false;
    auto& storage = registry.get_or_emplace<ScriptStorage>(entity);
    for (const auto& script : storage.Scripts)
        if (script.TypeName == name) return false;
    const auto* definition = Find(name);
    std::unique_ptr<ScriptableComponent> instance;
    if (definition) instance = definition->Factory();
    if (instance)
    {
        instance->m_CurrentScene = &scene;
        instance->m_EntityHandle = entity;
    }
    storage.Scripts.emplace_back(std::move(instance), false, definition ? definition->Factory : nullptr, definition ? definition->ModuleLease : nullptr);
    storage.Scripts.back().TypeName = name;
    if (!definition) FI_CORE_WARN("Script '{}' is not compiled or registered; preserving its data", name);
    return true;
}

void FINALITY::ScriptRegistry::ClearModule()
{
    for (const auto& name : s_ModuleNames) s_Definitions.erase(name);
    s_ModuleNames.clear();
}

bool FINALITY::ScriptRegistry::ReplaceModule(const std::vector<ScriptDefinition>& definitions, const std::shared_ptr<void>& lease, Scene* scene, std::string& error)
{
    try
    {
        auto candidate = s_Definitions;
        for (const auto& name : s_ModuleNames) candidate.erase(name);
        std::set<std::string> names;
        for (auto definition : definitions)
        {
            if (definition.Name.empty() || !definition.Factory || candidate.contains(definition.Name)) throw std::runtime_error("Duplicate or invalid module script type: " + definition.Name);
            std::set<std::string> properties;
            for (const auto& property : definition.Properties)
                if (property.Name.empty() || !property.Get || !property.Set || !properties.insert(property.Name).second) throw std::runtime_error("Invalid script property in " + definition.Name);
            definition.ModuleLease = lease;
            names.insert(definition.Name);
            candidate.emplace(definition.Name,std::move(definition));
        }
        struct Replacement
        {
            entt::entity Entity;
            size_t Index;
            ScriptStorage::ScriptInstance Value;
        };
        std::vector<Replacement> replacements;
        if (scene)
            for (auto entity : scene->GetRegistry().view<ScriptStorage>())
            {
                auto& storage = scene->GetRegistry().get<ScriptStorage>(entity);
                for (size_t index = 0; index < storage.Scripts.size(); ++index)
                {
                    const auto& old = storage.Scripts[index];
                    if (!s_ModuleNames.contains(old.TypeName) && !names.contains(old.TypeName)) continue;
                    auto values = old.Properties;
                    if (const auto* definition = Find(old.TypeName); definition && old.Instance)
                        for (const auto& property : definition->Properties) values[property.Name] = property.Get(*old.Instance);
                    const auto found = candidate.find(old.TypeName);
                    const auto* definition = found == candidate.end() ? nullptr : &found->second;
                    auto instance = definition ? definition->Factory() : nullptr;
                    if (definition && !instance) throw std::runtime_error("Script factory returned no instance: " + old.TypeName);
                    if (instance)
                    {
                        instance->m_CurrentScene = scene; instance->m_EntityHandle = entity;
                        for (const auto& property : definition->Properties)
                            if (const auto value = values.find(property.Name); value != values.end() && property.Get(*instance).index() == value->second.index()) property.Set(*instance,value->second);
                    }
                    ScriptStorage::ScriptInstance next(std::move(instance),false,definition ? definition->Factory : nullptr,definition ? definition->ModuleLease : nullptr);
                    next.TypeName = old.TypeName; next.Enabled = old.Enabled; next.Properties = std::move(values);
                    replacements.push_back({entity,index,std::move(next)});
                }
            }
        s_Definitions.swap(candidate);
        s_ModuleNames = std::move(names);
        std::vector<std::shared_ptr<ScriptableComponent>> retired;
        for (auto& replacement : replacements)
        {
            auto& old = scene->GetRegistry().get<ScriptStorage>(replacement.Entity).Scripts.at(replacement.Index);
            if (old.Instance && old.Awakened) { old.Awakened = false; retired.push_back(old.Instance); }
            std::swap(old,replacement.Value);
        }
        for (const auto& instance : retired)
            try { instance->OnDestroy(); }
            catch (const std::exception& failure) { FI_CORE_ERROR("Retired script destruction failed: {}",failure.what()); }
        return true;
    }
    catch (const std::exception& failure) { error = failure.what(); return false; }
}

void FINALITY::ScriptRegistry::Remove(Scene& scene, entt::entity entity, size_t index)
{
    auto* storage = scene.GetRegistry().try_get<ScriptStorage>(entity);
    if (!storage || index >= storage->Scripts.size()) return;
    auto instance = storage->Scripts[index].Instance;
    const bool awakened = storage->Scripts[index].Awakened;
    storage->Scripts.erase(storage->Scripts.begin() + index);
    if (instance && awakened) instance->OnDestroy();
}
