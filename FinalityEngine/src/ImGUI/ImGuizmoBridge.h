/**
 * @file
 * @ingroup tools
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ImGuizmoBridge.h.
 */

#pragma once

#include <Core/Core.h>
#include <glm/glm.hpp>

namespace FINALITY
{
    struct TransformComponent;

    /** @todo TODO: Document ImGuizmoBridge class semantics, ownership and usage. */
    class FAPI ImGuizmoBridge
    {
    public:
        static void BeginFrame();
        static bool IsActive();
        static bool Manipulate(const glm::mat4& view,
            const glm::mat4& projection,
            int operation,
            glm::mat4& worldTransform, bool local = false);

        static void DecomposeTransform(const glm::mat4& transform,
            glm::vec3& position,
            glm::vec3& rotationDegrees,
            glm::vec3& scale);
        static void SetLocalTransform(TransformComponent& component, const glm::mat4& local);
    };
}
