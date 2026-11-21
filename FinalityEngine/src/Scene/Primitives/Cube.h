/**
 * @file
 * @ingroup scene
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Cube.h.
 */
#pragma once
#include <memory>
#include <Core/Mesh.h>

namespace FINALITY::Primitives
{
    /** @todo TODO: Document Cube class semantics, ownership and usage. */
    class Cube
    {
    public:
        static const std::vector<Vertex>& GetVertices();
        static const std::vector<uint32_t>& GetIndices();
        static std::shared_ptr<Mesh> CreateMesh();
    };
}