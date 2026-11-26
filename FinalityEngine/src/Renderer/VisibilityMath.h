/**
 * @file
 * @ingroup render
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VisibilityMath.h.
 */
#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace FINALITY
{
    inline float ConservativeScale(const glm::mat4& transform)
    {
        float oneNorm = 0, infinityNorm = 0;
        for (int i = 0; i < 3; ++i)
        {
            float column = 0, row = 0;
            for (int j = 0; j < 3; ++j)
            {
                column += std::abs(transform[i][j]);
                row += std::abs(transform[j][i]);
            }
            oneNorm = std::max(oneNorm, column);
            infinityNorm = std::max(infinityNorm, row);
        }
        return std::sqrt(oneNorm * infinityNorm);
    }
} // namespace FINALITY
