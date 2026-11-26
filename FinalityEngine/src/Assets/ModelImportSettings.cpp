#include "ModelImportSettings.h"
#define YAML_CPP_STATIC_DEFINE
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <cmath>
#include <map>

namespace FINALITY::ModelImportSettings
{
    ModelImportOptions Read(const std::filesystem::path& model, const ModelImportOptions& defaults)
    {
        struct Entry
        {
            std::filesystem::file_time_type Time;
            YAML::Node Data;
        };

        static std::map<std::filesystem::path, Entry> cache;
        auto metadata = model;
        metadata += ".fmeta";
        if (!std::filesystem::is_regular_file(metadata))
        {
            return defaults;
        }
        const auto time = std::filesystem::last_write_time(metadata);
        auto found = cache.find(metadata);
        if (found == cache.end() || found->second.Time != time)
        {
            found = cache.insert_or_assign(metadata, Entry{ time, YAML::LoadFile(metadata.string())["ModelImporter"] }).first;
        }
        const auto& node = found->second.Data;
        if (!node || !node.IsMap())
        {
            return defaults;
        }
        auto result = defaults;
        result.Scale = node["Scale"].as<float>(defaults.Scale);
        result.ConvertUnits = node["ConvertUnits"].as<bool>(defaults.ConvertUnits);
        result.GenerateNormals = node["GenerateNormals"].as<bool>(defaults.GenerateNormals);
        result.FlipUVs = node["FlipUVs"].as<bool>(defaults.FlipUVs);
        result.ImportMaterials = node["ImportMaterials"].as<bool>(defaults.ImportMaterials);
        if (!std::isfinite(result.Scale) || result.Scale <= 0)
        {
            throw std::runtime_error("Invalid model import scale");
        }
        return result;
    }

    bool Save(const std::filesystem::path& model, const ModelImportOptions& options, std::string& error)
    {
        try
        {
            if (!std::isfinite(options.Scale) || options.Scale <= 0)
            {
                throw std::runtime_error("Scale must be finite and positive");
            }
            auto path = model;
            path += ".fmeta";
            auto metadata = YAML::LoadFile(path.string());
            if (!metadata["ID"])
            {
                throw std::runtime_error("Model has no asset identity; refresh assets first");
            }
            auto node = metadata["ModelImporter"];
            node["Version"] = 1;
            node["Scale"] = options.Scale;
            node["ConvertUnits"] = options.ConvertUnits;
            node["GenerateNormals"] = options.GenerateNormals;
            node["FlipUVs"] = options.FlipUVs;
            node["ImportMaterials"] = options.ImportMaterials;
            auto temporary = path;
            temporary += ".tmp";
            {
                std::ofstream stream(temporary);
                stream << metadata;
                if (!stream)
                {
                    throw std::runtime_error("Cannot save model import settings");
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
} // namespace FINALITY::ModelImportSettings
