/**
 * @file
 * @ingroup assets
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for MeshImporter.h.
 */
#pragma once

#include <Core/Mesh.h>

namespace FINALITY
{
    /** @todo TODO: Document MeshGeometry struct semantics, ownership and usage. */
    struct MeshGeometry
    {
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;
    };

    /** @todo TODO: Document MeshImporter class semantics, ownership and usage. */
    class FAPI MeshImporter
    {
    public:
        static MeshGeometry Load(const std::string& path);
        static MeshGeometry Primitive(const std::string& name);
    };
} // namespace FINALITY
