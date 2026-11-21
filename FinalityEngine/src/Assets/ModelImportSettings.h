/**
 * @file
 * @ingroup assets
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ModelImportSettings.h.
 */
#pragma once
#include "ModelImportOptions.h"
#include <filesystem>
#include <string>

namespace FINALITY::ModelImportSettings
{
    ModelImportOptions Read(const std::filesystem::path& model, const ModelImportOptions& defaults);
    bool Save(const std::filesystem::path& model, const ModelImportOptions& options, std::string& error);
}
