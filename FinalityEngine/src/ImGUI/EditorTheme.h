/**
 * @file
 * @ingroup tools
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for EditorTheme.h.
 */
#pragma once

#include <Core/Core.h>
#include <filesystem>
#include <imgui.h>
#include <string>
#include <vector>

namespace FINALITY
{
/** @todo TODO: Document ThemeChoice struct semantics, ownership and usage. */
struct ThemeChoice
{
    std::string ID;
    std::string Name;
};

/** @todo TODO: Document EditorTheme class semantics, ownership and usage. */
class FAPI EditorTheme
{
  public:
    static std::vector<ThemeChoice> List(const std::filesystem::path &engineAssets,
                                         const std::filesystem::path &project);
    static bool Load(const std::string &id, const std::filesystem::path &engineAssets,
                     const std::filesystem::path &project, ImGuiStyle &style, std::string &error);
    static bool Apply(const std::string &id, const std::filesystem::path &engineAssets,
                      const std::filesystem::path &project, std::string &error);
    static bool Export(const std::filesystem::path &path, const std::string &name, std::string &error);
};
}
