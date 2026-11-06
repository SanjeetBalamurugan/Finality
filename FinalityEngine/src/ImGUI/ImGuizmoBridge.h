
#pragma once

#include <Core/Core.h>
#include <glm/glm.hpp>

namespace FINALITY
{
    class FAPI ImGuizmoBridge
    {
    public:
        static bool Manipulate(const glm::mat4& view,
            const glm::mat4& projection,
            int operation,
            glm::mat4& worldTransform);

        static void DecomposeTransform(const glm::mat4& transform,
            glm::vec3& position,
            glm::vec3& rotationDegrees,
            glm::vec3& scale);
    };
}
