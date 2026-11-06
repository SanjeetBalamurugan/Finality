#pragma once
#include <memory>
#include <Core/Mesh.h>

namespace FINALITY::Primitives
{
    class Cube
    {
    public:
        static const std::vector<Vertex>& GetVertices();
        static const std::vector<uint32_t>& GetIndices();
        static std::shared_ptr<Mesh> CreateMesh();
    };
}