#include "LODGroup.h"
#include "Scene.h"
#include "Components.h"
#include <Renderer/VisibilityMath.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

void FINALITY::NormalizeLODGroup(LODGroupComponent& group)
{
    group.LocalSize = std::isfinite(group.LocalSize) ? std::max(0.001f, group.LocalSize) : 1;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (!std::isfinite(group.LocalCenter[axis]))
        {
            group.LocalCenter[axis] = 0;
        }
    }
    group.Hysteresis = std::isfinite(group.Hysteresis) ? std::clamp(group.Hysteresis, 0.0f, 0.5f) : 0.05f;
    if (group.Levels.size() > 8)
    {
        group.Levels.resize(8);
    }
    float previous = 1.001f;
    std::unordered_set<uint64_t> assigned;
    for (auto& level : group.Levels)
    {
        level.ScreenRelativeHeight = std::isfinite(level.ScreenRelativeHeight) ? std::clamp(level.ScreenRelativeHeight, 0.0f, std::max(0.0f, previous - 0.001f)) : std::max(0.0f, previous * 0.5f);
        previous = level.ScreenRelativeHeight;
        std::erase_if(level.Renderers, [&](uint64_t id)
                      {
                          return id == 0 || !assigned.insert(id).second;
                      });
    }
}

bool FINALITY::RecalculateLODBounds(Scene& scene, entt::entity entity)
{
    auto& registry = scene.GetRegistry();
    auto* group = registry.try_get<LODGroupComponent>(entity);
    if (!group)
    {
        return false;
    }
    const auto world = scene.GetWorldTransform(entity);
    if (std::abs(glm::determinant(world)) < 1e-8f)
    {
        return false;
    }
    const auto inverse = glm::inverse(world);
    std::unordered_set<uint64_t> ids;
    for (const auto& level : group->Levels)
    {
        ids.insert(level.Renderers.begin(), level.Renderers.end());
    }
    glm::vec3 minimum(std::numeric_limits<float>::max()), maximum(-minimum);
    bool found = false;
    for (auto renderer : registry.view<MeshComponent, TagComponent>())
    {
        if (!ids.contains(registry.get<TagComponent>(renderer).uuid))
        {
            continue;
        }
        const auto& mesh = registry.get<MeshComponent>(renderer).MeshData;
        if (!mesh)
        {
            continue;
        }
        const auto transform = inverse * scene.GetWorldTransform(renderer);
        const auto center = glm::vec3(transform * glm::vec4(mesh->GetBoundsCenter(), 1));
        const float radius = ConservativeScale(transform) * mesh->GetBoundsRadius();
        minimum = glm::min(minimum, center - glm::vec3(radius));
        maximum = glm::max(maximum, center + glm::vec3(radius));
        found = true;
    }
    if (!found)
    {
        return false;
    }
    group->LocalCenter = (minimum + maximum) * 0.5f;
    const auto dimensions = maximum - minimum;
    group->LocalSize = std::max({ dimensions.x, dimensions.y, dimensions.z });
    NormalizeLODGroup(*group);
    return true;
}
