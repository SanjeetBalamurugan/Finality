/**
 * @file
 * @ingroup assets
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ModelPipeline.h.
 */
#pragma once

#include "ModelAsset.h"

namespace FINALITY::ModelPipeline
{
    ModelData Read(const std::filesystem::path& source, const ModelImportOptions& options, const std::filesystem::path& resources);
    void Save(const ModelData& model, const std::filesystem::path& directory, const std::string& signature);
    bool ReadCache(ModelData& model, const std::filesystem::path& directory, const std::string& signature);
    void PrepareMaterials(ModelData& model, const std::filesystem::path& directory, const std::filesystem::path& engineAssets);
}
