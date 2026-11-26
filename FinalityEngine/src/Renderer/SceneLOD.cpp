#include "Renderer.h"
#include "VisibilityMath.h"
#include <Scene/Scene.h>
#include <unordered_map>
#include <limits>

void FINALITY::Renderer::ConfigureLODGroups(const Scene& scene)
{
    std::unordered_map<uint64_t, size_t> packets;
    for (size_t i = 0; i < s_RenderQueue.size(); ++i)
    {
        packets[s_RenderQueue[i].EntityID] = i;
    }
    const auto& registry = scene.GetRegistry();
    for (auto entity : registry.view<LODGroupComponent, TagComponent>())
    {
        auto component = registry.get<LODGroupComponent>(entity);
        if (!component.Enabled || component.Levels.empty())
        {
            continue;
        }
        NormalizeLODGroup(component);
        auto group = std::make_shared<RenderLODGroup>();
        group->ID = registry.get<TagComponent>(entity).uuid;
        group->Hysteresis = component.Hysteresis;
        const auto world = scene.GetWorldTransform(entity);
        group->Center = glm::vec3(world * glm::vec4(component.LocalCenter, 1));
        group->Size = component.LocalSize * ConservativeScale(world);
        glm::vec3 minimum(std::numeric_limits<float>::max()), maximum(-minimum);
        bool found = false;
        for (size_t level = 0; level < component.Levels.size(); ++level)
        {
            group->Thresholds.push_back(component.Levels[level].ScreenRelativeHeight);
            for (auto id : component.Levels[level].Renderers)
            {
                const auto item = packets.find(id);
                if (item == packets.end())
                {
                    continue;
                }
                auto& packet = s_RenderQueue[item->second];
                if (packet.LODGroup)
                {
                    continue;
                }
                packet.LODGroup = group;
                packet.LODLevel = static_cast<int>(level);
                const auto center = glm::vec3(packet.Transform * glm::vec4(packet.MeshData->GetBoundsCenter(), 1));
                const float radius = packet.MeshData->GetBoundsRadius() * ConservativeScale(packet.Transform);
                minimum = glm::min(minimum, center - glm::vec3(radius));
                maximum = glm::max(maximum, center + glm::vec3(radius));
                found = true;
            }
        }
        if (component.AutoBounds && found)
        {
            group->Center = (minimum + maximum) * 0.5f;
            const auto dimensions = maximum - minimum;
            group->Size = std::max({ dimensions.x, dimensions.y, dimensions.z });
        }
    }
}
