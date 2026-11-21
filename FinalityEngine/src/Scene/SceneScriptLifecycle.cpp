#include "Scene.h"
#include "ScriptableComponent.h"
#include <algorithm>
#include <chrono>
#include <cmath>

void FINALITY::Scene::SetTimeScale(float scale)
{
    if (std::isfinite(scale)) m_TimeScale = std::clamp(scale,0.0f,100.0f);
}

void FINALITY::Scene::SetFixedTimeStep(float step)
{
    if (std::isfinite(step) && step >= 0.001f && step <= 1)
    {
        m_FixedTimeStep = step;
        m_FixedAccumulator = 0;
    }
}

void FINALITY::Scene::OnUpdate(float ts)
{
    if (m_Dispatching) return;
    const auto begin = std::chrono::steady_clock::now();
    m_ScriptCalls = 0;
    m_FixedSteps = 0;
    m_ScriptSnapshot.clear();
    for (auto entity : m_EntityRegistry.view<ScriptStorage>())
        for (const auto& script : m_EntityRegistry.get<ScriptStorage>(entity).Scripts)
            if (script.Instance) m_ScriptSnapshot.emplace_back(entity,script.Instance);
    m_Dispatching = true;
    const auto find = [&](entt::entity entity, const std::shared_ptr<ScriptableComponent>& instance) -> ScriptStorage::ScriptInstance*
    {
        if (!m_EntityRegistry.valid(entity) || IsPendingDestroy(entity)) return nullptr;
        auto* storage = m_EntityRegistry.try_get<ScriptStorage>(entity);
        if (!storage) return nullptr;
        for (auto& entry : storage->Scripts) if (entry.Instance == instance) return &entry;
        return nullptr;
    };
    const auto ready = [&](entt::entity entity, const std::shared_ptr<ScriptableComponent>& instance)
    {
        auto* script = find(entity,instance);
        if (!script) return false;
        if (!script->Enabled)
        {
            if (script->Active) { script->Active = false; instance->OnDisable(); }
            return false;
        }
        if (script->Awakened && script->Active && script->Started) return true;
        if (!script->Awakened) { script->Awakened = true; instance->Awake(); }
        script = find(entity,instance);
        if (!script || !script->Enabled) return false;
        if (!script->Active) { script->Active = true; instance->OnEnable(); }
        script = find(entity,instance);
        if (!script || !script->Enabled) return false;
        if (!script->Started) { script->Started = true; instance->Start(); }
        script = find(entity,instance);
        return script && script->Enabled && script->Active;
    };
    try
    {
        for (const auto& [entity,instance] : m_ScriptSnapshot) ready(entity,instance);
        const float dt = std::isfinite(ts) ? std::clamp(ts,0.0f,0.25f)*m_TimeScale : 0;
        if (m_TimeScale > 0 && dt > 0)
        {
            m_FixedAccumulator += dt;
            while (m_FixedAccumulator+1e-9 >= m_FixedTimeStep && m_FixedSteps < 8)
            {
                const float step = m_FixedTimeStep;
                m_FixedAccumulator -= step;
                ++m_FixedSteps;
                for (const auto& [entity,instance] : m_ScriptSnapshot)
                    if (ready(entity,instance)) instance->FixedUpdate(step);
            }
            if (m_FixedSteps == 8 && m_FixedAccumulator >= m_FixedTimeStep)
                m_FixedAccumulator = std::fmod(m_FixedAccumulator,m_FixedTimeStep);
            for (const auto& [entity,instance] : m_ScriptSnapshot)
                if (ready(entity,instance)) { instance->Update(dt); ++m_ScriptCalls; }
            for (const auto& [entity,instance] : m_ScriptSnapshot)
                if (ready(entity,instance)) instance->LateUpdate(dt);
        }
        for (const auto& [entity,instance] : m_ScriptSnapshot)
            if (auto* entry = find(entity,instance); entry && entry->Active && !entry->Enabled)
            {
                entry->Active = false;
                instance->OnDisable();
            }
    }
    catch (...) { m_Dispatching = false; m_ScriptSnapshot.clear(); throw; }
    m_Dispatching = false;
    m_ScriptSnapshot.clear();
    auto mutations = std::move(m_ScriptMutations); m_ScriptMutations.clear();
    for (auto& mutation : mutations) mutation();
    auto destroyed = std::move(m_PendingDestroy); m_PendingDestroy.clear();
    for (auto entity : destroyed) DestroyEntity(entity);
    m_ScriptTimeMS = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
}
