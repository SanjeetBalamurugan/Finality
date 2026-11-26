#include "TextureImportSettings.h"
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <fstream>

FINALITY::TextureColorSpace FINALITY::ReadTextureColorSpace(const std::filesystem::path& texture)
{
    auto path = texture;
    path += ".fmeta";
    if (!std::filesystem::is_regular_file(path))
    {
        return TextureColorSpace::Automatic;
    }
    const auto node = YAML::LoadFile(path.string())["TextureImporter"];
    if (!node)
    {
        return TextureColorSpace::Automatic;
    }
    const auto name = node["ColorSpace"].as<std::string>("Automatic");
    if (name == "sRGB")
    {
        return TextureColorSpace::SRGB;
    }
    if (name == "Linear")
    {
        return TextureColorSpace::Linear;
    }
    return TextureColorSpace::Automatic;
}

bool FINALITY::SaveTextureColorSpace(const std::filesystem::path& texture, TextureColorSpace colorSpace, std::string& error)
{
    try
    {
        auto path = texture;
        path += ".fmeta";
        auto node = YAML::LoadFile(path.string());
        if (!node["ID"])
        {
            throw std::runtime_error("Refresh assets before changing texture import settings");
        }
        node["TextureImporter"]["ColorSpace"] = colorSpace == TextureColorSpace::SRGB ? "sRGB" : colorSpace == TextureColorSpace::Linear ? "Linear"
                                                                                                                                         : "Automatic";
        auto temporary = path;
        temporary += ".tmp";
        {
            std::ofstream stream(temporary);
            stream << node;
            if (!stream)
            {
                throw std::runtime_error("Cannot save texture import settings");
            }
        }
        std::filesystem::copy_file(temporary, path, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(temporary);
        error.clear();
        return true;
    }
    catch (const std::exception& failure)
    {
        error = failure.what();
        return false;
    }
}
