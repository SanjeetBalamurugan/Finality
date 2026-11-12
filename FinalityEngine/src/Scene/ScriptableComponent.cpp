#include "ScriptableComponent.h"

FINALITY::ScriptStorage::ScriptInstance::ScriptInstance(std::unique_ptr<ScriptableComponent> instance, bool started,
    std::function<std::unique_ptr<ScriptableComponent>()> factory, std::shared_ptr<void> moduleLease)
    : ModuleLease(std::move(moduleLease)), Started(started), Factory(std::move(factory))
{
    if (instance) Instance = std::shared_ptr<ScriptableComponent>(instance.release(),[lease = ModuleLease](ScriptableComponent* value) { delete value; });
}

FINALITY::ScriptStorage::ScriptInstance& FINALITY::ScriptStorage::ScriptInstance::operator=(ScriptInstance&& other) noexcept
{
    if (this == &other) return *this;
    auto oldLease = ModuleLease;
    Factory = std::move(other.Factory); Instance = std::move(other.Instance);
    Started = other.Started; Awakened = other.Awakened; Enabled = other.Enabled;
    TypeName = std::move(other.TypeName); Properties = std::move(other.Properties);
    ModuleLease = std::move(other.ModuleLease);
    return *this;
}
#include "Entity.h"

namespace FINALITY
{
    Entity ScriptableComponent::GetEntity()
    {
        return Entity{ m_EntityHandle, m_CurrentScene };
    }

    const Entity ScriptableComponent::GetEntity() const
    {
        return Entity{ m_EntityHandle, m_CurrentScene };
    }
}
