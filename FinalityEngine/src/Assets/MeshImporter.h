#pragma once

#include <Core/Mesh.h>

namespace FINALITY
{
    struct MeshGeometry
    {
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;
    };

    class FAPI MeshImporter
    {
    public:
        static MeshGeometry Load(const std::string& path);
        static MeshGeometry Primitive(const std::string& name);
    };
}
