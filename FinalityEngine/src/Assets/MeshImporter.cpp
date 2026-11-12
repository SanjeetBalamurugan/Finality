#include "MeshImporter.h"
#include <Scene/Primitives/Cube.h>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    void Face(FINALITY::MeshGeometry& geometry, uint32_t a, uint32_t b, uint32_t c)
    {
        const auto& vertices = geometry.Vertices;
        const auto normal = glm::cross(vertices[b].Position - vertices[a].Position, vertices[c].Position - vertices[a].Position);
        const auto expected = vertices[a].Normal + vertices[b].Normal + vertices[c].Normal;
        if (glm::dot(normal, normal) < 1.0e-12f) return;
        if (glm::dot(normal, expected) < 0) std::swap(b, c);
        geometry.Indices.insert(geometry.Indices.end(), { a, b, c });
    }
}

FINALITY::MeshGeometry FINALITY::MeshImporter::Primitive(const std::string& name)
{
    MeshGeometry geometry;
    if (name == "cube")
    {
        geometry.Vertices = Primitives::Cube::GetVertices();
        geometry.Indices = Primitives::Cube::GetIndices();
        Mesh::CalculateNormalsAndTangents(geometry.Vertices, geometry.Indices);
        return geometry;
    }
    if (name == "plane" || name == "quad")
    {
        geometry.Vertices = {
            {{-0.5f, -0.5f, 0}, {1,1,1}, {0,1}},
            {{ 0.5f, -0.5f, 0}, {1,1,1}, {1,1}},
            {{ 0.5f,  0.5f, 0}, {1,1,1}, {1,0}},
            {{-0.5f,  0.5f, 0}, {1,1,1}, {0,0}}
        };
        if (name == "plane")
            for (auto& vertex : geometry.Vertices)
                vertex.Position = { vertex.Position.x * 10, 0, -vertex.Position.y * 10 };
        geometry.Indices = {0,1,2,2,3,0};
        Mesh::CalculateNormalsAndTangents(geometry.Vertices, geometry.Indices);
        return geometry;
    }
    if (name != "sphere" && name != "capsule" && name != "cylinder")
        throw std::runtime_error("Unknown primitive: " + name);
    const uint32_t segments = 32;
    const uint32_t rings = name == "cylinder" ? 1 : 24;
    for (uint32_t ring = 0; ring <= rings; ++ring)
    {
        const float v = static_cast<float>(ring) / rings;
        const float latitude = Pi * v;
        float radius = name == "cylinder" ? 0.5f : std::sin(latitude) * 0.5f;
        float y = name == "cylinder" ? 0.5f - v : std::cos(latitude) * 0.5f;
        if (name == "capsule") y += ring <= rings / 2 ? 0.5f : -0.5f;
        for (uint32_t segment = 0; segment <= segments; ++segment)
        {
            const float u = static_cast<float>(segment) / segments;
            const float angle = u * Pi * 2;
            Vertex vertex{{radius * std::sin(angle), y, radius * std::cos(angle)}, {1,1,1}, {u,v}};
            vertex.Normal = name == "cylinder" ? glm::vec3(std::sin(angle),0,std::cos(angle)) :
                glm::vec3(std::sin(latitude) * std::sin(angle), std::cos(latitude), std::sin(latitude) * std::cos(angle));
            vertex.Tangent = {std::cos(angle), 0, -std::sin(angle), 1};
            geometry.Vertices.push_back(vertex);
        }
    }
    for (uint32_t ring = 0; ring < rings; ++ring)
        for (uint32_t segment = 0; segment < segments; ++segment)
        {
            const auto a = ring * (segments + 1) + segment;
            const auto b = a + segments + 1;
            Face(geometry, a, b, a + 1);
            Face(geometry, a + 1, b, b + 1);
        }
    if (name == "cylinder")
        for (int side : {-1, 1})
        {
            const uint32_t center = static_cast<uint32_t>(geometry.Vertices.size());
            Vertex vertex{{0, side * 0.5f, 0}, {1,1,1}, {0.5f,0.5f}};
            vertex.Normal = {0,side,0};
            geometry.Vertices.push_back(vertex);
            for (uint32_t segment = 0; segment <= segments; ++segment)
            {
                const float angle = static_cast<float>(segment) / segments * Pi * 2;
                vertex.Position = {std::sin(angle) * 0.5f, side * 0.5f, std::cos(angle) * 0.5f};
                vertex.TexCoord = {std::sin(angle) * 0.5f + 0.5f, std::cos(angle) * 0.5f + 0.5f};
                geometry.Vertices.push_back(vertex);
                if (segment) Face(geometry, center, center + segment, center + segment + 1);
            }
        }
    return geometry;
}

FINALITY::MeshGeometry FINALITY::MeshImporter::Load(const std::string& path)
{
    if (path.starts_with("builtin://")) return Primitive(path.substr(10));
    std::ifstream stream(path);
    if (!stream) throw std::runtime_error("Cannot open model: " + path);
    MeshGeometry geometry;
    std::vector<glm::vec3> positions, normals;
    std::vector<glm::vec2> coordinates;
    std::vector<glm::vec3> authoredNormals;
    auto index = [](int value, size_t size) -> size_t
    {
        const auto resolved = value > 0 ? static_cast<int64_t>(value) - 1 : static_cast<int64_t>(size) + value;
        if (!value || resolved < 0 || resolved >= static_cast<int64_t>(size)) throw std::runtime_error("OBJ index is outside its data array");
        return static_cast<size_t>(resolved);
    };
    std::string line;
    while (std::getline(stream, line))
    {
        std::istringstream input(line);
        std::string type;
        input >> type;
        if (type == "v" || type == "vn")
        {
            glm::vec3 value;
            if (!(input >> value.x >> value.y >> value.z) || !std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
                throw std::runtime_error("Invalid OBJ vector");
            (type == "v" ? positions : normals).push_back(value);
        }
        else if (type == "vt")
        {
            glm::vec2 value;
            if (!(input >> value.x >> value.y)) throw std::runtime_error("Invalid OBJ UV");
            coordinates.push_back(value);
        }
        else if (type == "f")
        {
            std::vector<uint32_t> face;
            std::string item;
            while (input >> item)
            {
                if (item.starts_with('#')) break;
                const auto first = item.find('/');
                const auto second = first == std::string::npos ? first : item.find('/', first + 1);
                Vertex vertex{positions.at(index(std::stoi(item), positions.size())), {1,1,1}, {0,0}};
                glm::vec3 normal(0);
                if (first != std::string::npos && first + 1 < item.size() && item[first + 1] != '/')
                    vertex.TexCoord = coordinates.at(index(std::stoi(item.substr(first + 1)), coordinates.size()));
                if (second != std::string::npos && second + 1 < item.size())
                    normal = normals.at(index(std::stoi(item.substr(second + 1)), normals.size()));
                face.push_back(static_cast<uint32_t>(geometry.Vertices.size()));
                geometry.Vertices.push_back(vertex);
                authoredNormals.push_back(normal);
            }
            if (face.size() < 3) throw std::runtime_error("OBJ face needs at least three vertices");
            for (size_t i = 1; i + 1 < face.size(); ++i)
                geometry.Indices.insert(geometry.Indices.end(), {face[0], face[i], face[i + 1]});
        }
    }
    if (geometry.Indices.empty()) throw std::runtime_error("Model has no triangle geometry");
    Mesh::CalculateNormalsAndTangents(geometry.Vertices, geometry.Indices);
    for (size_t i = 0; i < geometry.Vertices.size(); ++i)
        if (glm::dot(authoredNormals[i], authoredNormals[i]) > 1.0e-12f)
        {
            auto& vertex = geometry.Vertices[i];
            vertex.Normal = glm::normalize(authoredNormals[i]);
            const glm::vec3 projected = glm::vec3(vertex.Tangent) - vertex.Normal * glm::dot(vertex.Normal, glm::vec3(vertex.Tangent));
            if (glm::dot(projected, projected) > 1.0e-12f) vertex.Tangent = glm::vec4(glm::normalize(projected), vertex.Tangent.w);
        }
    return geometry;
}
