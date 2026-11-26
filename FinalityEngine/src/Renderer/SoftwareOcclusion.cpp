#include "SoftwareOcclusion.h"
#include "VisibilityMath.h"
#include <Core/Mesh.h>
#include <Core/Pipeline.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

namespace
{
    constexpr int Width = 128, Height = 72;
    constexpr size_t TriangleBudget = 24000;
    constexpr size_t PixelBudget = 100000;
    constexpr float Bias = 0.00001f;

    float Edge(glm::vec2 a, glm::vec2 b, glm::vec2 p)
    {
        return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
    }

    bool Project(const glm::vec4& clip, glm::vec3& screen)
    {
        if (!std::isfinite(clip.w) || clip.w < 0.00001f || clip.z < 0 || clip.z > clip.w)
        {
            return false;
        }
        screen = glm::vec3(clip) / clip.w;
        screen.x = (screen.x * 0.5f + 0.5f) * Width;
        screen.y = (0.5f - screen.y * 0.5f) * Height;
        return std::isfinite(screen.x) && std::isfinite(screen.y) && std::isfinite(screen.z);
    }

    void RasterTriangle(std::array<float, Width * Height>& depths, glm::vec3 a, glm::vec3 b, glm::vec3 c, FINALITY::CullMode culling, size_t& pixels)
    {
        float area = Edge(a, b, c);
        if (std::abs(area) < 0.00001f)
        {
            return;
        }
        if (culling == FINALITY::CullMode::Back && area > 0)
        {
            return;
        }
        if (culling == FINALITY::CullMode::Front && area < 0)
        {
            return;
        }
        if (area < 0)
        {
            std::swap(b, c);
            area = -area;
        }
        const int left = static_cast<int>(std::clamp(std::floor(std::min({ a.x, b.x, c.x })), 0.0f, float(Width)));
        const int right = static_cast<int>(std::clamp(std::ceil(std::max({ a.x, b.x, c.x })), 0.0f, float(Width)));
        const int top = static_cast<int>(std::clamp(std::floor(std::min({ a.y, b.y, c.y })), 0.0f, float(Height)));
        const int bottom = static_cast<int>(std::clamp(std::ceil(std::max({ a.y, b.y, c.y })), 0.0f, float(Height)));
        for (int y = top; y < bottom && pixels < PixelBudget; ++y)
        {
            for (int x = left; x < right && pixels < PixelBudget; ++x, ++pixels)
            {
                float farthest = 0;
                bool covered = true;
                const glm::vec2 sampleOffsets[] =
                {
                    glm::vec2(0, 0),
                    glm::vec2(1, 0),
                    glm::vec2(0, 1),
                    glm::vec2(1, 1)
                };

                for (const auto offset : sampleOffsets)
                {
                    const glm::vec2 point = glm::vec2(x, y) + offset;
                    const float u = Edge(b, c, point), v = Edge(c, a, point), w = Edge(a, b, point);
                    if (std::min({ u, v, w }) < 0.00001f)
                    {
                        covered = false;
                        break;
                    }
                    farthest = std::max(farthest, (u * a.z + v * b.z + w * c.z) / area);
                }
                if (covered)
                {
                    depths[y * Width + x] = std::min(depths[y * Width + x], farthest + Bias);
                }
            }
        }
    }

    bool Hidden(const FINALITY::RenderPacket& packet, const glm::mat4& vp, const std::array<float, Width * Height>& depths)
    {
        const auto center = glm::vec3(packet.Transform * glm::vec4(packet.MeshData->GetBoundsCenter(), 1));
        const float radius = packet.MeshData->GetBoundsRadius() * FINALITY::ConservativeScale(packet.Transform);
        glm::vec3 minimum(float(Width), float(Height), 1), maximum(0);
        for (int corner = 0; corner < 8; ++corner)
        {
            const auto p = center + radius * glm::vec3(corner & 1 ? 1 : -1, corner & 2 ? 1 : -1, corner & 4 ? 1 : -1);
            glm::vec3 screen;
            if (!Project(vp * glm::vec4(p, 1), screen))
            {
                return false;
            }
            minimum = glm::min(minimum, screen);
            maximum = glm::max(maximum, screen);
        }
        const int left = static_cast<int>(std::clamp(std::floor(minimum.x), 0.0f, float(Width)));
        const int right = static_cast<int>(std::clamp(std::ceil(maximum.x), 0.0f, float(Width)));
        const int top = static_cast<int>(std::clamp(std::floor(minimum.y), 0.0f, float(Height)));
        const int bottom = static_cast<int>(std::clamp(std::ceil(maximum.y), 0.0f, float(Height)));
        if (left >= right || top >= bottom)
        {
            return false;
        }
        for (int y = top; y < bottom; ++y)
        {
            for (int x = left; x < right; ++x)
            {
                if (depths[y * Width + x] >= minimum.z - Bias)
                {
                    return false;
                }
            }
        }
        return true;
    }
} // namespace

void FINALITY::ApplySoftwareOcclusion(std::vector<RenderPacket>& packets, const RenderView& view, RenderStatistics& statistics)
{
    const auto start = std::chrono::steady_clock::now();
    std::array<float, Width * Height> depths;
    depths.fill(1);
    const auto vp = view.Projection * view.View;
    size_t triangles = 0;
    size_t pixels = 0;
    for (const auto& packet : packets)
    {
        if (!packet.Occluder || packet.EditorOverlay || packet.PipelineInstance->IsTransparent() || !packet.PipelineInstance->WritesDepth() || !packet.PipelineInstance->TestsDepth())
        {
            continue;
        }
        const auto mvp = vp * packet.Transform;
        const auto& vertices = packet.MeshData->GetVertices();
        const auto& indices = packet.MeshData->GetIndices();
        const size_t count = indices.empty() ? vertices.size() : indices.size();
        for (size_t index = 0; index + 2 < count && triangles < TriangleBudget && pixels < PixelBudget; index += 3, ++triangles)
        {
            glm::vec3 screen[3];
            bool valid = true;
            for (int v = 0; v < 3; ++v)
            {
                const size_t vertex = indices.empty() ? index + v : indices[index + v];
                if (vertex >= vertices.size() || !Project(mvp * glm::vec4(vertices[vertex].Position, 1), screen[v]))
                {
                    valid = false;
                    break;
                }
            }
            if (valid)
            {
                RasterTriangle(depths, screen[0], screen[1], screen[2], packet.PipelineInstance->GetCullMode(), pixels);
            }
        }
        if (triangles >= TriangleBudget || pixels >= PixelBudget)
        {
            break;
        }
    }
    statistics.OccluderTriangles = triangles;
    if (triangles)
    {
        std::erase_if(packets, [&](const RenderPacket& packet)
                      {
                          if (packet.EditorOverlay || !packet.PipelineInstance->TestsDepth() || !Hidden(packet, vp, depths))
                          {
                              return false;
                          }
                          ++statistics.OcclusionCulled;
                          ++statistics.Culled;
                          --statistics.Visible;
                          return true;
                      });
    }
    statistics.OcclusionMS = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}
