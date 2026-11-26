/**
 * @file
 * @ingroup editor
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VisualStudioIntegration.h.
 */
#pragma once
#include <filesystem>

namespace FINALITY
{
    bool OpenInRunningSolution(const std::filesystem::path& solution, const std::filesystem::path& source);
}
