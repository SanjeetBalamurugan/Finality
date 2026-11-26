#include "ModelAsset.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>
#include <cstring>
#include <algorithm>

namespace
{
    std::vector<char> ReadBytes(const std::filesystem::path& path)
    {
        const auto size = std::filesystem::file_size(path);
        if (size > 512ull * 1024 * 1024)
        {
            throw std::runtime_error("Model resource exceeds the import size limit");
        }
        std::vector<char> bytes(static_cast<size_t>(size));
        std::ifstream stream(path, std::ios::binary);
        stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!stream)
        {
            throw std::runtime_error("Cannot read model resource: " + path.string());
        }
        return bytes;
    }

    std::string Base64(const std::vector<char>& bytes)
    {
        const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string output;
        output.reserve((bytes.size() + 2) / 3 * 4);
        for (size_t i = 0; i < bytes.size(); i += 3)
        {
            const auto a = static_cast<unsigned char>(bytes[i]);
            const auto b = i + 1 < bytes.size() ? static_cast<unsigned char>(bytes[i + 1]) : 0;
            const auto c = i + 2 < bytes.size() ? static_cast<unsigned char>(bytes[i + 2]) : 0;
            output += alphabet[a >> 2];
            output += alphabet[((a & 3) << 4) | (b >> 4)];
            output += i + 1 < bytes.size() ? alphabet[((b & 15) << 2) | (c >> 6)] : '=';
            output += i + 2 < bytes.size() ? alphabet[c & 63] : '=';
        }
        return output;
    }

    std::string DecodeURI(const std::string& text)
    {
        std::string output;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '%' && i + 2 < text.size())
            {
                const auto pair = text.substr(i + 1, 2);
                if (pair.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
                {
                    throw std::runtime_error("Invalid resource URI");
                }
                output += static_cast<char>(std::stoul(pair, nullptr, 16));
                i += 2;
            }
            else
            {
                output += text[i];
            }
        }
        return output;
    }
} // namespace

void FINALITY::ModelAsset::MakePortable(const std::filesystem::path& path, const std::filesystem::path& sourceDirectory)
{
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    if (extension != ".gltf" && extension != ".glb")
    {
        return;
    }
    const auto bytes = ReadBytes(path);
    std::string json;
    std::vector<char> remainder;
    if (extension == ".glb")
    {
        uint32_t header[5]{};
        if (bytes.size() < sizeof(header))
        {
            throw std::runtime_error("Truncated GLB header");
        }
        std::memcpy(header, bytes.data(), sizeof(header));
        if (header[0] != 0x46546c67 || header[1] != 2 || header[2] != bytes.size() || header[4] != 0x4e4f534a || header[3] > bytes.size() - 20)
        {
            throw std::runtime_error("Invalid GLB container");
        }
        json.assign(bytes.data() + 20, header[3]);
        remainder.assign(bytes.begin() + 20 + header[3], bytes.end());
    }
    else
    {
        json.assign(bytes.begin(), bytes.end());
    }
    rapidjson::Document document;
    document.Parse(json.data(), json.size());
    if (document.HasParseError() || !document.IsObject())
    {
        throw std::runtime_error("Invalid glTF JSON");
    }
    bool changed = false;
    constexpr const char* collections[] =
    {
        "buffers",
        "images"
    };

    for (const auto* collection : collections)
    {
        if (!document.HasMember(collection) || !document[collection].IsArray())
        {
            continue;
        }
        for (auto& resource : document[collection].GetArray())
        {
            if (!resource.IsObject() || !resource.HasMember("uri") || !resource["uri"].IsString())
            {
                continue;
            }
            const std::string uri = resource["uri"].GetString();
            if (uri.starts_with("data:"))
            {
                continue;
            }
            if (uri.find("://") != std::string::npos)
            {
                throw std::runtime_error("Remote model resources must be imported locally");
            }
            const auto file = (sourceDirectory.empty() ? path.parent_path() : sourceDirectory) / DecodeURI(uri);
            const auto data = ReadBytes(file);
            std::string mime = "application/octet-stream";
            if (std::string(collection) == "images")
            {
                mime = file.extension() == ".jpg" || file.extension() == ".jpeg" ? "image/jpeg" : "image/png";
                if (resource.HasMember("mimeType") && resource["mimeType"].IsString())
                {
                    mime = resource["mimeType"].GetString();
                }
            }
            const auto embedded = "data:" + mime + ";base64," + Base64(data);
            resource["uri"].SetString(embedded.c_str(), static_cast<rapidjson::SizeType>(embedded.size()), document.GetAllocator());
            changed = true;
        }
    }
    if (!changed)
    {
        return;
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    std::string output(buffer.GetString(), buffer.GetSize());
    const auto temporary = path.string() + ".portable.tmp";
    std::ofstream stream(temporary, std::ios::binary);
    if (extension == ".glb")
    {
        while (output.size() % 4)
        {
            output += ' ';
        }
        const uint32_t header[] = { 0x46546c67, 2, static_cast<uint32_t>(20 + output.size() + remainder.size()), static_cast<uint32_t>(output.size()), 0x4e4f534a };
        stream.write(reinterpret_cast<const char*>(header), sizeof(header));
        stream.write(output.data(), output.size());
        stream.write(remainder.data(), remainder.size());
    }
    else
    {
        stream.write(output.data(), output.size());
    }
    stream.close();
    if (!stream)
    {
        throw std::runtime_error("Cannot write portable model source");
    }
    std::filesystem::copy_file(temporary, path, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(temporary);
}
