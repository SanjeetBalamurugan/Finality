#include "ModelPipeline.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <fstream>
#include <algorithm>
#include <cfloat>
#include <iomanip>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include "AssetRegistry.h"

namespace
{
    glm::mat4 Matrix(const aiMatrix4x4& source)
    {
        glm::mat4 result;
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column) result[column][row] = source[row][column];
        return result;
    }

    std::string Name(std::string value)
    {
        for (auto& c : value) if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') c = '_';
        if (value.empty()) value = "mesh";
        return value.substr(0,80);
    }

    std::string ExtractTexture(const aiScene& scene, const aiString& name, const std::filesystem::path& source,
        const std::filesystem::path& directory, unsigned int material, std::vector<std::string>& diagnostics)
    {
        namespace fs = std::filesystem;
        if (directory.empty()) return name.C_Str();
        fs::create_directories(directory);
        if (const auto* texture = scene.GetEmbeddedTexture(name.C_Str()))
        {
            const std::string format = texture->mHeight ? "tga" : texture->achFormatHint[0] ? texture->achFormatHint : "png";
            const auto target = directory / ("embedded_" + std::to_string(material) + "." + format);
            std::ofstream stream(target, std::ios::binary);
            if (!texture->mHeight) stream.write(reinterpret_cast<const char*>(texture->pcData), texture->mWidth);
            else
            {
                if (texture->mWidth > 65535 || texture->mHeight > 65535) throw std::runtime_error("Embedded texture dimensions are too large");
                unsigned char header[18]{};
                header[2] = 2; header[12] = texture->mWidth & 255; header[13] = texture->mWidth >> 8;
                header[14] = texture->mHeight & 255; header[15] = texture->mHeight >> 8; header[16] = 32; header[17] = 0x28;
                stream.write(reinterpret_cast<const char*>(header),18);
                stream.write(reinterpret_cast<const char*>(texture->pcData), static_cast<std::streamsize>(texture->mWidth) * texture->mHeight * 4);
            }
            if (!stream) throw std::runtime_error("Cannot extract embedded model texture");
            return target.string();
        }
        fs::path texture(name.C_Str());
        if (!texture.is_absolute()) texture = source.parent_path() / texture;
        if (!fs::is_regular_file(texture)) texture = source.parent_path() / fs::path(name.C_Str()).filename();
        if (!fs::is_regular_file(texture))
        {
            const auto assets = FINALITY::AssetRegistry::GetProjectDirectory() / "Assets";
            for (auto directory = source.parent_path(); !directory.empty(); directory = directory.parent_path())
            {
                for (const auto* folder : {"Textures", "textures"})
                {
                    const auto candidate = directory / folder / fs::path(name.C_Str()).filename();
                    if (fs::is_regular_file(candidate)) { texture = candidate; break; }
                }
                if (fs::is_regular_file(texture) || directory == assets || directory == directory.parent_path()) break;
            }
        }
        const auto target = directory / ("texture_" + std::to_string(material) + "_" + fs::path(name.C_Str()).filename().string());
        if (!fs::is_regular_file(texture))
        {
            if (fs::is_regular_file(target)) return target.string();
            diagnostics.push_back("Missing texture: " + std::string(name.C_Str()));
            return {};
        }
        if (!fs::equivalent(texture.parent_path(), directory) || texture.filename() != target.filename())
            fs::copy_file(texture, target, fs::copy_options::overwrite_existing);
        return target.string();
    }
}

FINALITY::ModelData FINALITY::ModelPipeline::Read(const std::filesystem::path& source, const ModelImportOptions& options, const std::filesystem::path& resources)
{
    Assimp::Importer importer;
    unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_ValidateDataStructure | aiProcess_SortByPType;
    if (options.GenerateNormals) flags |= aiProcess_GenSmoothNormals;
    if (options.FlipUVs) flags |= aiProcess_FlipUVs;
    const auto* scene = importer.ReadFile(source.string(), flags);
    if (!scene || !scene->mRootNode || !scene->mNumMeshes) throw std::runtime_error(importer.GetErrorString());
    ModelData output;
    std::map<std::string,int> names;
    for (unsigned int index = 0; index < scene->mNumMeshes; ++index)
    {
        const auto& mesh = *scene->mMeshes[index];
        ModelMesh part;
        part.Name = mesh.mName.length ? mesh.mName.C_Str() : "Mesh " + std::to_string(index);
        const auto key = Name(part.Name);
        part.Key = key + "_" + std::to_string(names[key]++);
        part.Material = mesh.mMaterialIndex;
        part.Geometry.Vertices.reserve(mesh.mNumVertices);
        for (unsigned int vertex = 0; vertex < mesh.mNumVertices; ++vertex)
        {
            const auto& position = mesh.mVertices[vertex];
            Vertex value{{position.x,position.y,position.z},{1,1,1},{0,0}};
            if (mesh.HasVertexColors(0)) value.Color = {mesh.mColors[0][vertex].r,mesh.mColors[0][vertex].g,mesh.mColors[0][vertex].b};
            if (mesh.HasTextureCoords(0)) value.TexCoord = {mesh.mTextureCoords[0][vertex].x,1.0f - mesh.mTextureCoords[0][vertex].y};
            if (mesh.HasNormals()) value.Normal = {mesh.mNormals[vertex].x,mesh.mNormals[vertex].y,mesh.mNormals[vertex].z};
            if (mesh.HasTangentsAndBitangents())
            {
                const auto& tangent = mesh.mTangents[vertex]; const auto& bitangent = mesh.mBitangents[vertex];
                value.Tangent = {tangent.x,tangent.y,tangent.z,glm::dot(glm::cross(value.Normal,glm::vec3(tangent.x,tangent.y,tangent.z)),glm::vec3(bitangent.x,bitangent.y,bitangent.z)) < 0 ? -1.0f : 1.0f};
            }
            part.Geometry.Vertices.push_back(value);
        }
        for (unsigned int face = 0; face < mesh.mNumFaces; ++face)
        {
            const auto& triangle = mesh.mFaces[face];
            if (triangle.mNumIndices != 3) continue;
            part.Geometry.Indices.insert(part.Geometry.Indices.end(), triangle.mIndices, triangle.mIndices + 3);
        }
        if (mesh.HasBones()) output.Diagnostics.push_back(part.Name + ": bone metadata remains in the source asset; static geometry is used");
        output.Meshes.push_back(std::move(part));
    }
    for (unsigned int index = 0; index < scene->mNumMaterials; ++index)
    {
        const auto& material = *scene->mMaterials[index];
        ModelMaterial value;
        aiString name;
        material.Get(AI_MATKEY_NAME, name);
        value.Name = name.length ? name.C_Str() : "Material " + std::to_string(index);
        aiColor4D color(1,1,1,1);
        if (aiGetMaterialColor(&material, AI_MATKEY_BASE_COLOR, &color) != AI_SUCCESS) aiGetMaterialColor(&material, AI_MATKEY_COLOR_DIFFUSE, &color);
        value.Color = {color.r,color.g,color.b,color.a};
        int twoSided = 0;
        material.Get(AI_MATKEY_TWOSIDED,twoSided); value.DoubleSided = twoSided != 0;
        for (unsigned int property = 0; property < material.mNumProperties; ++property)
        {
            const auto& data = *material.mProperties[property];
            std::ostringstream bytes;
            for (unsigned int i = 0; i < data.mDataLength; ++i) bytes << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(data.mData[i]));
            value.Properties[std::string(data.mKey.C_Str()) + ":" + std::to_string(data.mSemantic) + ":" + std::to_string(data.mIndex)] = std::to_string(data.mType) + ":" + bytes.str();
        }
        aiString texture;
        if (material.GetTexture(aiTextureType_BASE_COLOR,0,&texture) != AI_SUCCESS) material.GetTexture(aiTextureType_DIFFUSE,0,&texture);
        if (texture.length)
        {
            value.Texture = ExtractTexture(*scene,texture,source,resources,index,output.Diagnostics);
            if (!value.Texture.empty()) output.Dependencies.push_back(value.Texture);
        }
        output.Materials.push_back(std::move(value));
    }
    auto visit = [&](auto&& self, const aiNode& node, int parent) -> void
    {
        if (output.Nodes.size() > 100000) throw std::runtime_error("Model hierarchy is too large");
        const int index = static_cast<int>(output.Nodes.size());
        ModelNode value;
        value.Name = node.mName.length ? node.mName.C_Str() : "Node";
        value.Parent = parent;
        value.Transform = Matrix(node.mTransformation);
        if (parent == -1) value.Transform = glm::scale(glm::mat4(1),glm::vec3(options.Scale)) * value.Transform;
        if (node.mNumMeshes) value.Meshes.assign(node.mMeshes,node.mMeshes + node.mNumMeshes);
        output.Nodes.push_back(std::move(value));
        for (unsigned int child = 0; child < node.mNumChildren; ++child) self(self,*node.mChildren[child],index);
    };
    visit(visit,*scene->mRootNode,-1);
    output.Minimum = glm::vec3(FLT_MAX); output.Maximum = glm::vec3(-FLT_MAX);
    std::vector<glm::mat4> world(output.Nodes.size(),glm::mat4(1));
    for (size_t index = 0; index < output.Nodes.size(); ++index)
    {
        const auto& node = output.Nodes[index];
        world[index] = node.Parent < 0 ? node.Transform : world.at(node.Parent) * node.Transform;
        for (auto mesh : node.Meshes)
            for (const auto& vertex : output.Meshes.at(mesh).Geometry.Vertices)
            {
                const auto position = glm::vec3(world[index] * glm::vec4(vertex.Position,1));
                output.Minimum = glm::min(output.Minimum,position); output.Maximum = glm::max(output.Maximum,position);
            }
    }
    return output;
}
