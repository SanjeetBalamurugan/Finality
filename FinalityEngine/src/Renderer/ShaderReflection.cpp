#include "ShaderReflection.h"
#include "ShaderCompiler.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <spirv_cross/spirv_cross.hpp>
#include <stdexcept>

namespace
{
std::string TypeName(const spirv_cross::SPIRType &type)
{
    using Type = spirv_cross::SPIRType;
    std::string name;
    switch (type.basetype)
    {
    case Type::Float:
        name = "float";
        break;
    case Type::Int:
        name = "int";
        break;
    case Type::UInt:
        name = "uint";
        break;
    case Type::Boolean:
        name = "bool";
        break;
    case Type::Double:
        name = "double";
        break;
    case Type::Struct:
        name = "struct";
        break;
    default:
        name = "resource";
        break;
    }
    if (type.columns > 1)
        name = "mat" + std::to_string(type.columns) + "x" + std::to_string(type.vecsize);
    else if (type.vecsize > 1)
        name += std::to_string(type.vecsize);
    for (auto count : type.array)
        name += "[" + std::to_string(count) + "]";
    return name;
}
struct Cached
{
    std::filesystem::file_time_type Modified;
    std::vector<FINALITY::ShaderUniform> Uniforms;
};
std::map<std::string, Cached> s_Layouts;
}

std::vector<FINALITY::ShaderUniform> FINALITY::ShaderReflection::Reflect(const std::string &source)
{
    if (!ShaderCompiler::EnsureCompiled(source))
        throw std::runtime_error("Shader could not be compiled");
    const auto path = std::filesystem::absolute(source + ".spv").lexically_normal();
    const auto modified = std::filesystem::last_write_time(path);
    if (const auto found = s_Layouts.find(path.string());
        found != s_Layouts.end() && found->second.Modified == modified)
        return found->second.Uniforms;
    const auto size = std::filesystem::file_size(path);
    if (!size || size % 4 || size > 64 * 1024 * 1024)
        throw std::runtime_error("Invalid SPIR-V binary");
    std::vector<uint32_t> words(static_cast<size_t>(size / 4));
    std::ifstream binary(path, std::ios::binary);
    binary.read(reinterpret_cast<char *>(words.data()), static_cast<std::streamsize>(size));
    if (!binary)
        throw std::runtime_error("Cannot read SPIR-V binary");
    spirv_cross::Compiler compiler(std::move(words));
    const auto resources = compiler.get_shader_resources();
    std::vector<ShaderUniform> uniforms;
    auto block = [&](const spirv_cross::Resource &resource, const char *storage) {
        const auto &type = compiler.get_type(resource.base_type_id);
        const bool push = std::string(storage) == "Push constant";
        const auto set = push ? 0u : compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const auto binding = push ? 0u : compiler.get_decoration(resource.id, spv::DecorationBinding);
        for (uint32_t member = 0; member < type.member_types.size(); ++member)
        {
            const auto &value = compiler.get_type(type.member_types[member]);
            ShaderUniform uniform;
            uniform.Name = compiler.get_member_name(type.self, member);
            if (uniform.Name.empty())
                uniform.Name = "Member " + std::to_string(member);
            uniform.Block = resource.name;
            uniform.Storage = storage;
            uniform.Type = TypeName(value);
            uniform.Offset = compiler.type_struct_member_offset(type, member);
            uniform.Size = static_cast<uint32_t>(compiler.get_declared_struct_member_size(type, member));
            uniform.Set = set;
            uniform.Binding = binding;
            uniform.Editable = push && uniform.Offset >= 64 && uniform.Size <= 128 && uniform.Offset <= 128 - uniform.Size &&
                               (uniform.Type == "float" || uniform.Type == "float4" || uniform.Type == "mat4x4");
            uniforms.push_back(std::move(uniform));
        }
    };
    for (const auto &resource : resources.push_constant_buffers)
        block(resource, "Push constant");
    for (const auto &resource : resources.uniform_buffers)
        block(resource, "Uniform buffer");
    for (const auto &resource : resources.storage_buffers)
        block(resource, "Storage buffer");
    for (const auto &resource : resources.sampled_images)
    {
        ShaderUniform uniform;
        uniform.Name = resource.name;
        uniform.Type = "sampler2D";
        uniform.Storage = "Texture";
        uniform.Set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uniform.Binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        uniforms.push_back(std::move(uniform));
    }
    if (s_Layouts.size() > 256)
        s_Layouts.clear();
    s_Layouts[path.string()] = {modified, uniforms};
    return uniforms;
}
