/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ScenePlacement.h.
 */
#pragma once
#include <Finality.h>
#include <imgui.h>
#include <limits>
#include <cmath>

namespace FINALITY
{
    /** @todo TODO: Document SceneRay struct semantics, ownership and usage. */
    struct SceneRay { glm::vec3 Origin, Direction; };

    inline SceneRay ViewportRay(const Camera& camera, ImVec2 minimum, ImVec2 maximum, ImVec2 mouse)
    {
        const float x = 2*(mouse.x-minimum.x)/(maximum.x-minimum.x)-1;
        const float y = 1-2*(mouse.y-minimum.y)/(maximum.y-minimum.y);
        const auto inverse = glm::inverse(camera.GetProjection()*camera.GetViewMatrix());
        const auto nearPoint = inverse*glm::vec4(x,y,0,1);
        const auto farPoint = inverse*glm::vec4(x,y,1,1);
        const auto origin = glm::vec3(nearPoint)/nearPoint.w;
        return {origin,glm::normalize(glm::vec3(farPoint)/farPoint.w-origin)};
    }

    inline glm::vec3 SceneDropPosition(const Camera& camera, ImVec2 minimum, ImVec2 maximum, ImVec2 mouse)
    {
        const auto ray = ViewportRay(camera,minimum,maximum,mouse);
        if (std::abs(ray.Direction.y) > 0.0001f)
        {
            const float distance = -ray.Origin.y/ray.Direction.y;
            if (distance > 0 && distance < 1000) return ray.Origin+ray.Direction*distance;
        }
        return ray.Origin+ray.Direction*10.0f;
    }

    inline bool HierarchyBounds(const Scene& scene, entt::entity root, glm::vec3& minimum, glm::vec3& maximum)
    {
        minimum = glm::vec3(std::numeric_limits<float>::max()); maximum = -minimum;
        bool found = false;
        const auto& registry = scene.GetRegistry();
        for (auto entity : registry.view<MeshComponent>())
        {
            auto parent = entity;
            for (size_t depth = 0; parent != entt::null && registry.valid(parent) && depth < 1024; ++depth)
            {
                if (parent == root)
                {
                    const auto& mesh = registry.get<MeshComponent>(entity).MeshData;
                    if (!mesh) break;
                    const auto world = scene.GetWorldTransform(entity);
                    const auto center = glm::vec3(world*glm::vec4(mesh->GetBoundsCenter(),1));
                    const float radius = mesh->GetBoundsRadius();
                    const glm::vec3 extent = radius*(glm::abs(glm::vec3(world[0]))+glm::abs(glm::vec3(world[1]))+glm::abs(glm::vec3(world[2])));
                    minimum = glm::min(minimum,center-extent); maximum = glm::max(maximum,center+extent); found = true;
                    break;
                }
                const auto* relationship = registry.try_get<RelationshipComponent>(parent);
                parent = relationship ? relationship->Parent : entt::null;
            }
        }
        return found;
    }
}
